# POTCAWERecompVulkan - static recompilation of Pirates of the Caribbean: At World's End (Xbox 360),
# built on the ReXGlue SDK, drawn by NativeXRenderer (native_x_renderer/, a submodule).
#
# The SDK is the thirdparty/rexglue-sdk git submodule: ElGuslml/rexglue-sdk, branch recomp, the official
# nightly of 2026-10-02 (v0.10.0 and 24 later commits) with the changes these projects and NativeXRenderer
# need, one commit each (RECOMP.md there).
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(potcawe LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Same CPU baseline as the official SDK builds (its win-amd64 preset): x86-64-v2, i.e. SSE4.2 and
# POPCNT on top of the SSSE3/SSE4.1 the SDK code requires. Not -march=native, so the binaries run on
# any x86-64-v2 CPU.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
    add_compile_options(-march=x86-64-v2)
endif()

# Build the SDK from the submodule. RECOMP.md marks the recomp branch: a checkout of the official SDK lacks
# its changes and would not build this project.
set(REXSDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rexglue-sdk" CACHE PATH
    "Path to the rexglue-sdk source tree" FORCE)
if(NOT EXISTS "${REXSDK_DIR}/RECOMP.md")
    message(FATAL_ERROR
        "The ReXGlue SDK submodule (${REXSDK_DIR}) is missing or not on the recomp branch of "
        "ElGuslml/rexglue-sdk. Run: git submodule sync && git submodule update --init --recursive --force")
endif()

# Both graphics backends: D3D12 and Vulkan.
if(WIN32)
    set(REXGLUE_USE_D3D12 ON CACHE BOOL "Enable the D3D12 graphics backend")
endif()
set(REXGLUE_USE_VULKAN ON CACHE BOOL "Enable the Vulkan graphics backend")

include(generated/rexglue.cmake)

# The first build of a fresh tree runs the codegen, which writes generated/default/sources.cmake;
# CMake has to configure again to pick up the generated sources (otherwise the link fails with
# PPCImageConfig). A marker file holds the sources.cmake this configure saw and is a configure
# dependency; it is refreshed with copy_if_different after the codegen, so it only changes (and
# triggers a new configure) when the list of generated sources changes. Ninja brings configure
# dependencies up to date before building anything else, so the first build runs the codegen,
# configures again and goes on within the same command. (The codegen stamp itself cannot be the
# dependency: the codegen touches it on every run.)
set(POTCAWE_CODEGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated/default")
set(POTCAWE_SOURCES_MARKER "${POTCAWE_CODEGEN_DIR}/sources.cmake.configured")
if(EXISTS "${POTCAWE_CODEGEN_DIR}/sources.cmake")
    file(COPY_FILE "${POTCAWE_CODEGEN_DIR}/sources.cmake" "${POTCAWE_SOURCES_MARKER}" ONLY_IF_DIFFERENT)
elseif(NOT EXISTS "${POTCAWE_SOURCES_MARKER}")
    file(WRITE "${POTCAWE_SOURCES_MARKER}" "")
endif()
add_custom_command(
    OUTPUT "${POTCAWE_SOURCES_MARKER}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${POTCAWE_CODEGEN_DIR}/sources.cmake" "${POTCAWE_SOURCES_MARKER}"
    DEPENDS "${POTCAWE_CODEGEN_DIR}/codegen.build.stamp"
    COMMENT "Checking the list of generated sources"
    VERBATIM)
add_custom_target(potcawe_sources_marker DEPENDS "${POTCAWE_SOURCES_MARKER}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${POTCAWE_SOURCES_MARKER}")

# Sources
set(POTCAWE_SOURCES
    src/main.cpp
    src/aspect.cpp
    src/views.cpp
    src/framerate.cpp
    src/msaa.cpp
    src/gpu_wait.cpp
    src/lua_errors.cpp
    src/options.cpp
    src/quit_prompt.cpp
    src/startup.cpp
    src/native_x_renderer_potcawe.cpp
)

if(WIN32)
    add_executable(potcawe WIN32 ${POTCAWE_SOURCES})
else()
    add_executable(potcawe ${POTCAWE_SOURCES})
endif()

rexglue_setup_target(potcawe GPU_PLUGINS xenos)
add_dependencies(potcawe potcawe_sources_marker)

# NativeXRenderer: the native renderer (native_x_renderer/README.md). The game side (hooks, profile) is in src/.
add_subdirectory(native_x_renderer)
target_link_libraries(potcawe PRIVATE native_x_renderer)
# NVIDIA's DLSS runtime next to the executable (Phase 5, D46).
native_x_renderer_copy_runtime(potcawe)

# The target and the app keep the name "potcawe" (potcawe.toml, logs\potcawe_NNN.log); only the
# executable gets a descriptive name.
set_target_properties(potcawe PROPERTIES OUTPUT_NAME "PiratesAtWorldsEndRecomp")

# With the SDK built from source, rexruntime does not pass the Dear ImGui include directory on to the
# host target (the installed SDK ships imgui.h in include/); rex_app.cpp and potcawe_app.h need it. The
# symbols still come from rexruntime.dll.
target_include_directories(potcawe PRIVATE "${REXSDK_DIR}/thirdparty/imgui")

# Game files: assets/ next to PiratesAtWorldsEndRecomp.exe (release layout) or, if absent, the
# repository's assets/.
target_compile_definitions(potcawe PRIVATE
    POTCAWE_DEFAULT_GAME_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/assets")

# PiratesAtWorldsEnd Launcher: game options in a separate executable so that PiratesAtWorldsEndRecomp.exe
# starts right away. It writes potcawe_options.toml next to PiratesAtWorldsEndRecomp.exe and starts it on
# "Play". The game's own names, files and options are launcher/game.h and game.cpp; the rest is the same
# launcher as recomp-tools' template. SDL3 and Dear ImGui from the SDK tree, linked statically (one
# executable, without the SDK's DLLs), drawn with Direct3D 11 (the SDK builds SDL3 without SDL_render);
# nanosvg draws its icons and art.
if(WIN32)
    add_executable(potcawe_launcher WIN32
        launcher/main.cpp
        launcher/app.cpp
        launcher/game.cpp
        launcher/settings.cpp
        launcher/texts.cpp
        launcher/page_play.cpp
        launcher/page_settings.cpp
        launcher/page_game.cpp
        launcher/page_keyboard.cpp
        launcher/page_gamepad.cpp
        launcher/page_updates.cpp
        launcher/page_achievements.cpp
        launcher/page_install.cpp
        launcher/iso.cpp
        launcher/install.cpp
        launcher/achievements.cpp
        launcher/net.cpp
        launcher/updater.cpp
        launcher/zip.cpp
        launcher/ui/gfx.cpp
        launcher/ui/ui.cpp
        "${REXSDK_DIR}/thirdparty/imgui/backends/imgui_impl_sdl3.cpp"
        "${REXSDK_DIR}/thirdparty/imgui/backends/imgui_impl_dx11.cpp")
    set_target_properties(potcawe_launcher PROPERTIES OUTPUT_NAME "PiratesAtWorldsEnd Launcher")
    target_compile_definitions(potcawe_launcher PRIVATE UNICODE _UNICODE NOMINMAX IMGUI_DEFINE_MATH_OPERATORS)
    target_include_directories(potcawe_launcher PRIVATE launcher res launcher/thirdparty/nanosvg
        "${REXSDK_DIR}/thirdparty/imgui/backends" "${REXSDK_DIR}/thirdparty/tomlplusplus"
        "${REXSDK_DIR}/thirdparty/inja/third_party/include" "${REXSDK_DIR}/thirdparty/stb")
    target_link_libraries(potcawe_launcher PRIVATE imgui SDL3::SDL3 d3d11 dxgi d3dcompiler shcore windowscodecs ole32
        winhttp bcrypt)
    # A development build's game files, as the game falls back to them: the launcher then skips the first run.
    target_compile_definitions(potcawe_launcher PRIVATE
        LAUNCHER_DEFAULT_GAME_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    # The embedded fonts and art (res/launcher.rc): a change rebuilds the resources.
    file(GLOB POTCAWE_LAUNCHER_RESOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/launcher/fonts/*.ttf" "${CMAKE_CURRENT_SOURCE_DIR}/launcher/art/*.svg")
    set_source_files_properties(res/launcher.rc PROPERTIES OBJECT_DEPENDS "${POTCAWE_LAUNCHER_RESOURCES}")
endif()

# Windows resources: file properties (res/version.h) and the icon, res/icon.ico, when it exists. The
# executable's first icon is also the one SDL and the launcher use for their windows.
if(WIN32)
    target_sources(potcawe PRIVATE res/game.rc)
    target_sources(potcawe_launcher PRIVATE res/launcher.rc)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/res/icon.ico")
        target_compile_definitions(potcawe PRIVATE POTCAWERECOMP_HAS_ICON)
        target_compile_definitions(potcawe_launcher PRIVATE POTCAWERECOMP_HAS_ICON)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/res/icon.ico")
    endif()
endif()

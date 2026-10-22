// PiratesAtWorldsEnd Launcher: options for potcawe (recompiled Pirates of the Caribbean: At World's End)
// in a separate executable, so that PiratesAtWorldsEndRecomp.exe starts right away (for example from Steam
// Big Picture).
//
// Launcher 2 (local_docs/10): Gustavo's mockup (local_docs/launcher_design) on SDL3 and Dear ImGui, drawn with
// Direct3D 11, all from the SDK tree and linked statically, so it is one executable without the SDK's DLLs.
// It reads potcawe.toml and potcawe_options.toml (next to this exe and PiratesAtWorldsEndRecomp.exe), saves
// every change to potcawe_options.toml at once (settings.cpp), which PiratesAtWorldsEndRecomp.exe loads at
// startup (src/options.cpp), and works with the mouse, the keyboard and any controller SDL knows (the
// DualSense included).
//
// The window scales with its size (the mockup's 1280x760 at 100 %) and fills the screen on the Steam Deck and
// in Big Picture; F11 or Alt+Enter switch it. "Play" starts PiratesAtWorldsEndRecomp.exe (with the arguments
// the launcher received) and waits for it to exit, so Steam keeps seeing the game as running. The texts
// follow the Windows display language (English, German, French, Spanish, Italian); the game asks for its own
// language in its first menu.

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellscalingapi.h>

#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED  // wWinMain is this file's; SDL is told with SDL_SetMainReady()
#include <SDL3/SDL_main.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_sdl3.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "app.h"
#include "ui/gfx.h"
#include "ui/ui.h"

namespace {

using launcher::App;

// The mockup's window and the narrowest layout the pages are made for, in dp: the scale follows the
// window's height, unless the window is narrower than that layout.
constexpr float kDesignWidth = 1280.0f;
constexpr float kDesignHeight = 760.0f;
constexpr float kLayoutWidth = 1200.0f;
// The smallest window, in dp.
constexpr float kMinWidth = 1000.0f;
constexpr float kMinHeight = 600.0f;

float ScaleFor(int width, int height) {
  float scale = std::min(float(height) / kDesignHeight, float(width) / kLayoutWidth);
  // In steps of 1/40: fonts and icons are drawn again only when it changes.
  return std::clamp(std::round(scale * 40.0f) / 40.0f, 0.5f, 4.0f);
}

bool EnvironmentIs(const char* name, const char* value) {
  char text[64];
  DWORD length = GetEnvironmentVariableA(name, text, sizeof(text));
  return length > 0 && length < sizeof(text) && std::string(text, length) == value;
}

// Steam sets SteamDeck=1 on the Steam Deck and SteamGamepadUI=1 in Big Picture (Proton passes both to the
// Windows environment): the launcher fills the screen there.
bool SteamWantsFullscreen() { return EnvironmentIs("SteamDeck", "1") || EnvironmentIs("SteamGamepadUI", "1"); }

std::filesystem::path ExecutableFolder() {
  std::wstring path(MAX_PATH, L'\0');
  for (;;) {
    DWORD length = GetModuleFileNameW(nullptr, path.data(), DWORD(path.size()));
    if (length < path.size()) {
      path.resize(length);
      break;
    }
    path.resize(path.size() * 2);
  }
  return std::filesystem::path(path).parent_path();
}

// What follows the program name on the command line, verbatim (with the leading space).
std::wstring ForwardedArguments() {
  const wchar_t* p = GetCommandLineW();
  if (*p == L'"') {
    ++p;
    while (*p && *p != L'"') {
      ++p;
    }
    if (*p) {
      ++p;
    }
  } else {
    while (*p && *p != L' ' && *p != L'\t') {
      ++p;
    }
  }
  return p;
}

void ShowError(const char* text) {
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "PiratesAtWorldsEnd Launcher", text, nullptr);
}

// Starts PiratesAtWorldsEndRecomp.exe and waits for it to exit: meanwhile Steam keeps seeing the game as
// running.
int LaunchGame(const std::filesystem::path& exe_dir, const launcher::Texts& t) {
  std::filesystem::path game = exe_dir / L"PiratesAtWorldsEndRecomp.exe";
  if (!std::filesystem::exists(game)) {
    ShowError(t.game_missing);
    return 1;
  }
  std::wstring command_line = L"\"" + game.wstring() + L"\"" + ForwardedArguments();
  STARTUPINFOW startup = {sizeof(startup)};
  PROCESS_INFORMATION process = {};
  AllowSetForegroundWindow(ASFW_ANY);
  if (!CreateProcessW(game.c_str(), command_line.data(), nullptr, nullptr, FALSE, 0, nullptr, exe_dir.c_str(),
                      &startup, &process)) {
    std::string text = "CreateProcess: " + std::to_string(GetLastError());
    ShowError(text.c_str());
    return 1;
  }
  CloseHandle(process.hThread);
  WaitForSingleObject(process.hProcess, INFINITE);
  DWORD exit_code = 0;
  GetExitCodeProcess(process.hProcess, &exit_code);
  CloseHandle(process.hProcess);
  return int(exit_code);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
  // Physical pixels everywhere (monitors, modes, window) and sharp with display scaling.
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  std::filesystem::path exe_dir = ExecutableFolder();
  launcher::Settings settings = launcher::LoadSettings(exe_dir);
  uint32_t language = launcher::SystemLanguage();
  const launcher::Texts& t = launcher::TextsFor(language);

  SDL_SetMainReady();
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    ShowError(SDL_GetError());
    return 1;
  }

  // On the monitor of the active window (Big Picture, Explorer...), centered, at the size it had (or the
  // mockup's) times the monitor's scale, and no larger than its work area.
  HMONITOR monitor = MonitorFromWindow(GetForegroundWindow(), MONITOR_DEFAULTTOPRIMARY);
  MONITORINFO monitor_info = {sizeof(monitor_info)};
  GetMonitorInfoW(monitor, &monitor_info);
  const RECT& work = monitor_info.rcWork;
  int work_width = work.right - work.left;
  int work_height = work.bottom - work.top;
  float dpi_scale = float(GetDpiForSystem()) / 96.0f;
  if (UINT dpi = 0; SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi, &dpi))) {
    dpi_scale = float(dpi) / 96.0f;
  }
  launcher::Size logical = settings.o.ui_size.first > 0 && settings.o.ui_size.second > 0
                               ? settings.o.ui_size
                               : launcher::Size{int(kDesignWidth), int(kDesignHeight)};
  float width = float(logical.first) * dpi_scale;
  float height = float(logical.second) * dpi_scale;
  float fit = std::min({1.0f, float(work_width) * 0.92f / width, float(work_height) * 0.92f / height});
  int window_width = int(width * fit);
  int window_height = int(height * fit);
  int min_width = std::min(int(kMinWidth * dpi_scale), work_width);
  int min_height = std::min(int(kMinHeight * dpi_scale), work_height);
  window_width = std::max(window_width, min_width);
  window_height = std::max(window_height, min_height);

  SDL_Window* window = SDL_CreateWindow("PiratesAtWorldsEnd Launcher", window_width, window_height,
                                        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
  if (!window) {
    ShowError(SDL_GetError());
    SDL_Quit();
    return 1;
  }
  SDL_SetWindowPosition(window, work.left + (work_width - window_width) / 2, work.top + (work_height - window_height) / 2);
  SDL_SetWindowMinimumSize(window, min_width, min_height);
  bool fullscreen = settings.o.ui_fullscreen || SteamWantsFullscreen() ||
                    work_width < int(kMinWidth * dpi_scale) || work_height < int(kMinHeight * dpi_scale);
  if (settings.o.ui_maximized) {
    SDL_MaximizeWindow(window);
  }
  if (fullscreen) {
    SDL_SetWindowFullscreen(window, true);
  }

  HWND hwnd = HWND(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
  if (!ui::gfx::Init(hwnd)) {
    ShowError(t.no_direct3d);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.LogFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::GetStyle().FontSizeBase = 16.0f;
  // The widgets draw their own focus ring (ui::FocusRing).
  ImGui::GetStyle().Colors[ImGuiCol_NavCursor] = ImVec4(0, 0, 0, 0);
  ImGui_ImplSDL3_InitForD3D(window);
  ImGui_ImplDX11_Init(ui::gfx::Device(), ui::gfx::Context());
  if (!ui::LoadFonts()) {
    ShowError("PiratesAtWorldsEnd Launcher: the fonts are missing from the executable.");
    return 1;
  }

  App app(std::move(settings), language);
  bool shown = false;
  bool primed = false;
  int idle_frames = 0;
  float scale = 0.0f;
  while (app.result() == App::Result::kNone) {
    // Nothing changed for a while: wait for input instead of drawing the same frame.
    if (idle_frames > 30 && !app.Busy()) {
      SDL_WaitEventTimeout(nullptr, 250);
    }
    bool input = false;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      input = true;
      if (event.type == SDL_EVENT_QUIT ||
          (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))) {
        app.Quit();
        break;
      }
      if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        ui::gfx::Resize(event.window.data1, event.window.data2);
      }
      if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat && !app.Listening() &&
          (event.key.key == SDLK_F11 || (event.key.key == SDLK_RETURN && (event.key.mod & SDL_KMOD_ALT)))) {
        fullscreen = !fullscreen;
        SDL_SetWindowFullscreen(window, fullscreen);
        if (!SteamWantsFullscreen()) {
          app.settings().o.ui_fullscreen = fullscreen;
          launcher::SaveOptions(app.settings());
        }
        continue;
      }
      if (app.OnEvent(event)) {
        continue;
      }
      ImGui_ImplSDL3_ProcessEvent(&event);
    }
    idle_frames = input ? 0 : idle_frames + 1;
    if (app.result() != App::Result::kNone) {
      break;
    }
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
      SDL_WaitEventTimeout(nullptr, 100);
      continue;
    }

    int pixel_width = 0;
    int pixel_height = 0;
    SDL_GetWindowSizeInPixels(window, &pixel_width, &pixel_height);
    if (float s = ScaleFor(pixel_width, pixel_height); s != scale) {
      scale = s;
      ui::SetScale(scale);
      ui::ReleasePictures();
    }
    // The controller navigates once the buttons held at the start are up, and not while a key is being
    // assigned (B cancels that, App::OnKeyCapture).
    // A controller button held when the window opened (the A that started the launcher from Big Picture) must
    // not press anything; neither must the one just assigned on the Gamepad page (App::GamepadLocked).
    if (!primed && !ui::AnyGamepadButtonDown()) {
      primed = true;
    }
    if (primed && !app.Listening() && !app.GamepadLocked()) {
      io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    } else {
      io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    app.Frame(ImVec2(float(pixel_width), float(pixel_height)));
    ImGui::Render();
    ui::gfx::BeginFrame(ui::col::kBackground);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    ui::gfx::Present();
    if (!shown) {
      SDL_ShowWindow(window);
      SDL_RaiseWindow(window);
      shown = true;
    }
  }

  // The window's size for the next time, in pixels at 100 % scaling.
  SDL_WindowFlags flags = SDL_GetWindowFlags(window);
  launcher::Options& o = app.settings().o;
  o.ui_maximized = (flags & SDL_WINDOW_MAXIMIZED) != 0;
  if (!(flags & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MINIMIZED))) {
    int w = 0;
    int h = 0;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    o.ui_size = {int(float(w) / dpi_scale + 0.5f), int(float(h) / dpi_scale + 0.5f)};
  }
  bool saved = launcher::SaveOptions(app.settings());
  App::Result result = app.result();

  ui::ReleasePictures();
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  ui::gfx::Shutdown();
  SDL_DestroyWindow(window);
  if (result != App::Result::kPlay) {
    SDL_Quit();
    return 0;
  }
  if (!saved) {
    ShowError(t.save_failed);
  }
  SDL_Quit();
  return LaunchGame(exe_dir, t);
}

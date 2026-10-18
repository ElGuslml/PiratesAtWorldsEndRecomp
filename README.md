# PiratesAtWorldsEndRecomp

A static recompilation of **Pirates of the Caribbean: At World's End** (Xbox 360, 2007) to native Windows,
built on the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk). The game's PowerPC code is translated to
C++ and compiled.

It is drawn by [**NativeXRenderer**](https://github.com/ElGuslml/NativeXRenderer), a native renderer on Vulkan.
NativeXRenderer draws the game's Direct3D calls on your GPU instead of emulating the Xbox 360's. ReXGlue's
emulation of that GPU, Xenos (derived from [Xenia](https://xenia.jp)), stays available as an alternative.

> [!IMPORTANT]
> **You need your own original copy of the game.** This project does not include the game's files, and never
> will: not its executable, levels, textures, sounds, videos or text.
> - To play, you need an Xbox 360 copy of *Pirates of the Caribbean: At World's End* that you own.
> - You extract its files yourself, from your own disc or from an image you made of it (see
>   [Game files](#game-files)).
> - Do not ask for the game's files, and do not share them or links to them.
>
> This repository contains no game files and no game code. The released executable contains the game's code
> recompiled for the PC, but none of its files: without yours, it does not run.
>
> This project is not affiliated with or endorsed by Disney, Eurocom or Microsoft.

> [!WARNING]
> Early release. The game has been played through its first levels, in sessions of an hour, without
> problems. Later levels have not been tested yet. Please report what you find.

## Playing

1. Download the latest release from [Releases](https://github.com/ElGuslml/PiratesAtWorldsEndRecomp/releases),
   and extract it into a folder you can write to (not under `Program Files`).
2. Copy your game files into its `assets` folder (see [Game files](#game-files)).
3. Run `PiratesAtWorldsEnd Launcher.exe`, choose your options, and press Play. Afterwards
   `PiratesAtWorldsEndRecomp.exe` starts the game directly with the saved options.

Requirements:
- **System:** Windows 10 or 11, 64-bit, with a CPU with x86-64-v2 (SSE4.2 and POPCNT: practically any 64-bit
  CPU from 2009 on).
- **GPU:** one with a Vulkan driver for NativeXRenderer. NVIDIA DLAA needs an RTX GPU. The Xenos path also
  runs on Direct3D 12.
- **Runtime:** the [Microsoft Visual C++ Redistributable 2015–2022 (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe).

## Game files

Everything the game needs besides this project's code comes from your own copy of the game:
1. Extract the content of your disc, or of an image (ISO) of your own disc, with
   [extract-xiso](https://github.com/XboxDev/extract-xiso/releases).
2. Copy all of it into the `assets` folder, so that `assets/default.xex` exists:
   - next to `PiratesAtWorldsEndRecomp.exe` to play;
   - in the repository to build.

The expected executable has CRC32 `A31B1069`. `PiratesAtWorldsEndRecomp.exe` checks it at startup and warns you
if yours is a different version. See [`assets/README.md`](assets/README.md).

This project does not provide the game's files, or links to them.

## Features

**NativeXRenderer**, the default renderer:
- **Resolution and aspect ratio:**
  - the game is drawn at your screen's resolution, or at a lower internal resolution you choose;
  - wide screens (21:9, 16:10) are filled by widening the game's 3D view, with the HUD, menus and videos
    kept at 16:9 in the middle.
- **Antialiasing of its own:** SMAA (the default), FXAA, or DLAA with NVIDIA DLSS on RTX GPUs.
- **Shaders:** a persistent cache. From the second run, nothing waits for shaders to compile in places you
  have already seen.
- **Fallback:** if NativeXRenderer cannot start, the game falls back to the Xenos path.

**The Xenos path** (launcher: Renderer → Xenos): ReXGlue's emulation of the Xbox 360 GPU, on Direct3D 12 or
Vulkan, at a whole multiple of 1280×720 (2×, 3×). Wider screens get black bars.

**Fixes for both** (each one has a short note in [`docs/`](docs/README.md)):
- **60 FPS option:** the game is capped at 30 FPS only by its Direct3D present interval; the engine's clock
  measures real time, so the option just presents every vblank
  ([`docs/frame-rate.md`](docs/frame-rate.md)).
- **Anisotropic filtering at 16×:** textures seen at an angle stay sharp.
- **Lower CPU use:** the game's render thread sleeps instead of spinning while it waits for the GPU
  ([`docs/cpu-usage.md`](docs/cpu-usage.md)).
- **A script error no longer closes the game.** The recompiled `longjmp` did not jump, so Lua treated even
  the errors the game catches as fatal. The log now names each script error
  ([`docs/script-errors.md`](docs/script-errors.md)).
- **On the Xenos path:**
  - no stutter the first time an effect appears ([`docs/stutters.md`](docs/stutters.md));
  - no flat-colored objects in Port Royal under GPU load
    ([`docs/flat-colored-objects.md`](docs/flat-colored-objects.md));
  - less GPU time at 2× and 3× ([`docs/gpu-load.md`](docs/gpu-load.md));
  - the sun's glare follows the sun ([`docs/sun-glow.md`](docs/sun-glow.md)).

## Configuration

**PiratesAtWorldsEnd Launcher** works with mouse, keyboard or controller. Its rows:
- monitor, display (fullscreen, borderless or window) and window size;
- renderer;
- with NativeXRenderer: image format (fill the screen, or 16:9 with bars) and internal resolution;
- with Xenos: render scale and graphics API;
- frame rate (30 or 60 FPS), antialiasing and anisotropic filtering.

Save writes the options and keeps the launcher open; Play saves them and starts the game.

| File (next to `PiratesAtWorldsEndRecomp.exe`) | What it holds |
|---|---|
| `potcawe.toml` | Base settings. Created with the defaults this game needs if it is missing. |
| `potcawe_options.toml` | What PiratesAtWorldsEnd Launcher saves. Loaded after `potcawe.toml`. |

Command-line options (`--name=value`, for example in Steam's launch options) win over both files. For
example:
- `--fullscreen=false --window_width=1920 --window_height=1080`;
- `--potcawe_60fps=true`;
- `--native_x_renderer=false` (the Xenos path);
- `--native_x_renderer_antialiasing=dlaa`.

Other locations:
- **Saves, the profile and the shader caches** go to `userdata\` next to the executable. If that folder
  cannot be written, they go to `Documents\potcawe`.
- **Logs** go to `logs\`.
- **Game files** are read from `assets\` next to the executable; `--game_data_root=<folder>` points
  somewhere else.

## Building from source

Requirements:
- Git;
- [CMake](https://cmake.org/) 3.25 or newer and [Ninja](https://ninja-build.org/);
- [LLVM/Clang](https://github.com/llvm/llvm-project/releases): tested with Clang 23, and MSVC alone does not
  build this;
- the Windows SDK;
- Python 3;
- your own copy of the game.

```powershell
git clone --recursive https://github.com/ElGuslml/PiratesAtWorldsEndRecomp.git
cd PiratesAtWorldsEndRecomp
# copy your game files into assets\ first: the build recompiles assets\default.xex
cmake --preset win-amd64-release
cmake --build --preset win-amd64-release
.\out\build\win-amd64-release\"PiratesAtWorldsEnd Launcher.exe"
```

- **Submodules:** the ReXGlue SDK (`thirdparty/rexglue-sdk`) and NativeXRenderer (`native_x_renderer`, with
  DiligentCore inside it). If you cloned without `--recursive`: `git submodule update --init --recursive`.
- **Clone into a short path,** such as `C:\src`. The submodules nest deeply, and in a long folder Windows'
  260-character path limit stops git with "'$GIT_DIR' too big". `git config --global core.longpaths true` also
  avoids it.
- **Configuring** applies the SDK fixes in [`patches/`](patches/) and NativeXRenderer's own SDK and
  DiligentCore patches.
- **The first build** translates `default.xex` into C++ (`generated/`) and builds the SDK: it takes a while.
  If it stops with "precompiled header ... has been modified", run the build command again.

## How the SDK is modified

`thirdparty/rexglue-sdk` is the official ReXGlue SDK at v0.10.0. The fixes are in [`patches/`](patches/), a
`git format-patch` series applied at configure time by
[`cmake/apply_sdk_patches.cmake`](cmake/apply_sdk_patches.cmake). They come from GRAWRecomp (Ghost Recon
Advanced Warfighter), where they were found; none of them is specific to that game, and the options they add
are off by default. Each patch's commit message explains the change. NativeXRenderer adds its own SDK patches
after these (its `sdk_patches/`).

| Patch | Change |
|---|---|
| 0001 | Keep host FP exceptions masked when the game updates the FPSCR |
| 0002 | Invalidate GPU copies of newly allocated physical memory |
| 0003 | Code generator: `vpkd3d128` lost the sign when the destination aliases the source |
| 0004 | `gpu_depth_clamp`: clamp depth instead of clipping in Z, like Xenos (D3D12 and Vulkan) |
| 0005 | `fullscreen_borderless_window` option |
| 0006 | `gpu_backend` option to choose D3D12 or Vulkan |
| 0007 | `XGetLanguage` honors `user_language` |
| 0008 | No busy waiting when there is no work (timer queue, presenter repaints) |
| 0009 | `gpu_async_pipelines_no_frame_wait`: do not stall the frame on new pipelines |
| 0010–0013 | Faster register and shader-constant writes from the command processor |
| 0014 | Log the guest context on unhandled guest access violations |
| 0015 | `gpu_guest_fps_limit`: optional frame rate cap for the game |
| 0016 | `XamUserReadProfileSettings` no longer writes past the game's buffer (title-specific settings larger than their nominal size) |
| 0017 | `render_target_skip_transfers_on_overwrite`: no EDRAM ownership transfer into a render target that a single triangle, rectangle or quad fully overwrites (the game's clears through MSAA / pitch aliases) |
| 0018 | `render_target_draw_clears`: a draw that writes constant values to a rectangle of its render targets is done as a clear of the render targets owning that EDRAM, never through the alias |
| 0019 | `d3d12_lazy_resolve`: color resolves kept as host render target copies and textures filled from them by copying, the shared memory written only on demand. Off: it measured no gain on an RTX 4070 Ti (the two host copies cost what the SDK's three passes cost) |
| 0020 | Occlusion queries: the end pairs with the begin whatever its address and in every EDRAM tile, so host queries are no longer turned off at the first query (D3D12 and Vulkan) |
| 0021 | SPIR-V: cube maps sampled at a LOD computed in the shader, not with gradients (NVIDIA's Vulkan driver returned garbage in one pixel of each quad: GRAW's false colors on the Vulkan `fbo` path and with NativeXRenderer; from GRAWRecomp), and the LOD bias scale kept finite |
| 0022 | Codegen: with `non_volatile_as_local` the elided `__savegprlr_N` still stores the return address in the frame's LR slot, which the guest's stack walks (and NativeXRenderer's caller checks) read (from GRAWRecomp) |
| 0023 | `WAIT_REG_MEM` polls every 100 µs with the high-resolution timer instead of sleeping 1 ms (from GRAWRecomp, which waits once a frame for a word the CPU clears) |
| 0024 | CMake: rexruntime and the GPU plugins are copied next to the executable on every build, not only when it links again (a change to the SDK alone left the old DLLs there) |
| 0025 | Codegen: a `share_registers` function (an SEH funclet, a setjmp caller) keeps its registers across calls: a caller that hands its own through `ctx` puts back what `ctx` held, and the register save/restore helpers are done on `ctx` instead of elided. Here the callers of Lua's protected call took back r27-r30 from it (from GRAWRecomp) |

## Documentation

- [`docs/`](docs/README.md): a short note on each problem found and what was done about it, plus
  [`native-x-renderer-potcawe.md`](docs/native-x-renderer-potcawe.md), the game side of NativeXRenderer (its
  Direct3D map, engine behavior and measurements).
- [NativeXRenderer](https://github.com/ElGuslml/NativeXRenderer): the renderer's design, work plan and
  decision log.

## Credits

- [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk), whose runtime is derived from [Xenia](https://xenia.jp).
- [DiligentCore](https://github.com/DiligentGraphics/DiligentCore), the graphics layer NativeXRenderer draws
  with.
- NVIDIA DLSS, for DLAA.
- [extract-xiso](https://github.com/XboxDev/extract-xiso), for extracting the game files.
- NativeXRenderer's design follows [Hell's Gate](https://github.com/florinp93/hells-gate-recomp) (Dante's
  Inferno on ReXGlue with a DiligentCore renderer) and
  [Unleashed Recompiled](https://github.com/hedge-dev/UnleashedRecomp); no code was taken from them.
- Developed by Gustavo Garcia with the help of Claude (Anthropic).

## License

MIT for this project's own code (see [`LICENSE`](LICENSE)). The ReXGlue SDK, NativeXRenderer and their
components keep their own licenses (see [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)). NVIDIA and DLSS
are trademarks or registered trademarks of NVIDIA Corporation. Pirates of the Caribbean: At World's End, its
code and its assets belong to their respective owners.

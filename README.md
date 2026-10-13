# POTCAWERecomp

A static recompilation of **Pirates of the Caribbean: At World's End** (Xbox 360, 2007) to native Windows,
built on the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk). The game's PowerPC code is translated to
C++ and compiled; the Xbox 360 GPU is emulated by ReXGlue's Xenos backend (derived from
[Xenia](https://xenia.jp)).

> [!WARNING]
> Early work in progress: the game boots and reaches gameplay, but only the first minutes have been tested.
> There are no releases.

> This project is not affiliated with Disney or Eurocom. It contains no game files and no game code: you need
> your own legally obtained copy of the game.

## Status

- `default.xex` is recompiled and runs with the default settings (Direct3D 12, 30 FPS, 16:9): language
  selection, title screen, main menu, the intro video and the start of gameplay.
- Fixes so far: two functions the code analyzer missed (`potcawe_functions.toml`) and SDK patch 0016 (see
  below).
- Lower CPU usage: the game's render thread sleeps instead of spinning while it waits for the GPU
  (`src/gpu_wait.cpp`; `potcawe_gpu_wait_sleep_us = 0` restores the original busy wait).
- No stutter the first time an effect appears: new D3D12 pipelines are created without stalling the frame
  (`gpu_async_pipelines_no_frame_wait = true` in `potcawe.toml`).
- No flat-colored objects in Port Royal under GPU load (60 FPS with a 2× or 3× render scale): the shaders'
  control flow is translated with `if` blocks instead of a `switch` (`dxbc_switch = false` in `potcawe.toml`,
  see [`docs/flat-colored-objects.md`](docs/flat-colored-objects.md)).
- Less GPU time per frame with a 2× or 3× render scale (2×2: 49 → 60 FPS with the GPU at 89 % instead of
  99 %; 3×3: 22 → 37 FPS on an RTX 4070 Ti): the game's 19 EDRAM clears per frame, drawn through
  render targets with another MSAA mode and pitch, are done as clears of the render targets that hold
  that memory instead of copying the frame into the alias and back (SDK patches 0017 and 0018 and
  `execute_unclipped_draw_vs_on_cpu = true` in `potcawe.toml`, see [`docs/gpu-load.md`](docs/gpu-load.md)).
- The sun's glare follows the sun: the game measures how much of the sun is visible with an occlusion query,
  which ReXGlue now answers with the host GPU's count instead of a fixed "fully visible" (SDK patch 0020, see
  [`docs/sun-glow.md`](docs/sun-glow.md)).
- **POTCAWE Launcher**: monitor, fullscreen / borderless / window, window resolution, render quality and
  graphics API. Works with mouse, keyboard or controller. The game always renders at 1280×720; ReXGlue renders
  it at a whole multiple per axis (2×, 3×) and fits the image to the screen. "Automatic" picks the smallest
  scale that covers your screen. Wider screens get black bars for now (the camera keeps 16:9).
- **60 FPS option** (launcher, `potcawe_60fps`): the game is capped at 30 FPS only by its Direct3D present
  interval; the engine's clock measures real time, so the option just presents every vblank. Off by default,
  so the first run matches the console ([`docs/frame-rate.md`](docs/frame-rate.md)).
- **Antialiasing option** (launcher, `potcawe_no_msaa`): the game renders with 2× MSAA into surfaces that do
  not fit the console's 10 MB of EDRAM, so it draws the whole scene twice (two tiles). On the PC that memory is
  virtual: "Off (faster)" creates the surfaces without multisampling and in one tile, which cuts the GPU time
  by a third at a 2×2 render scale (89 % → 59 % at 60 FPS on an RTX 4070 Ti) at the cost of the 2× edge
  antialiasing. Off by default, so the first run matches the console ([`docs/gpu-load.md`](docs/gpu-load.md)).
- Next: play further and find out which settings and fixes the rest of the game needs.
- [`docs/`](docs/README.md) has a short note on each problem found and what was done about it.

## Requirements

To build:
- Git, [CMake](https://cmake.org/) 3.25 or newer, [Ninja](https://ninja-build.org/),
  [LLVM/Clang](https://github.com/llvm/llvm-project/releases) (tested with Clang 23; MSVC alone does not build
  this) and the Windows SDK.
- Python 3 (glslang needs it to configure the Vulkan backend).
- Your own copy of the game (see below).

To run:
- Windows 10 or 11, 64-bit, with a GPU that supports Direct3D 12 (or Vulkan).
- A CPU with x86-64-v2 support (SSE4.2 and POPCNT: practically any 64-bit CPU from 2009 on).
- [Microsoft Visual C++ Redistributable 2015–2022 (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe).

## Game files

Extract the content of your disc or ISO with
[extract-xiso](https://github.com/XboxDev/extract-xiso/releases) and copy all of it into `assets/` so that
`assets/default.xex` exists. The expected executable has CRC32 `A31B1069`; `potcawerecomp.exe` warns you at
startup if yours is a different version. See [`assets/README.md`](assets/README.md).

## Building from source

```powershell
git clone --recursive https://github.com/ElGuslml/PiratesAtWorldsEndRecomp.git
cd PiratesAtWorldsEndRecomp
# copy your game files into assets\ first: the build recompiles assets\default.xex
cmake --preset win-amd64-release
cmake --build --preset win-amd64-release
```

- If you cloned without `--recursive`: `git submodule update --init --recursive`.
- Configuring applies the SDK fixes in [`patches/`](patches/) to `thirdparty/rexglue-sdk` (see below).
- The first build translates `default.xex` into C++ (`generated/`) and builds the SDK: it takes a while.
  If it stops with "precompiled header ... has been modified", run the build command again.

## Running

```powershell
.\out\build\win-amd64-release\"POTCAWE Launcher.exe"   # choose the options, then Play
.\out\build\win-amd64-release\potcawerecomp.exe        # or start the game directly with the saved options
```

- Game files are found in `assets\` next to `potcawerecomp.exe`, otherwise in the repository's `assets\`.
  Use `--game_data_root=<folder>` to point somewhere else.
- Saves, the profile and the shader cache go to `userdata\` next to `potcawerecomp.exe` (if that folder
  cannot be written, for example under Program Files, they go to `Documents\potcawe`). Logs go to `logs\`.

### Steam Big Picture

Add `potcawerecomp.exe` as a non-Steam game: it starts straight into the game with the options you saved in
POTCAWE Launcher. You can also add `POTCAWE Launcher.exe` to change options from the couch with a controller.

## Configuration

| File (next to `potcawerecomp.exe`) | What it holds |
|---|---|
| `potcawe.toml` | Base settings. Created with the defaults this game needs if it is missing. |
| `potcawe_options.toml` | What POTCAWE Launcher saves (monitor, display, window size, render scale, frame rate, antialiasing, API). Loaded after `potcawe.toml`. |

Command-line options (`--name=value`, for example Steam launch options) win over both files:
`--fullscreen=false --window_width=1920 --window_height=1080`,
`--draw_resolution_scale_x=2 --draw_resolution_scale_y=2`, `--potcawe_60fps=true`, `--potcawe_no_msaa=true`,
`--gpu_backend=vulkan`.

## How the SDK is modified

`thirdparty/rexglue-sdk` is the official ReXGlue SDK at v0.10.0. The fixes are in [`patches/`](patches/), a
`git format-patch` series applied at configure time by
[`cmake/apply_sdk_patches.cmake`](cmake/apply_sdk_patches.cmake). They come from
GRAWRecomp (Ghost Recon Advanced Warfighter), where they were found; none of them is specific to that game,
and the options they add are off by default. Each patch's commit message explains the change:

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

## Credits

- [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk), whose runtime is derived from [Xenia](https://xenia.jp).
- [extract-xiso](https://github.com/XboxDev/extract-xiso), for extracting the game files.
- Developed by Gustavo Garcia with the help of Claude (Anthropic).

## License

MIT for this project's own code (see [`LICENSE`](LICENSE)). The ReXGlue SDK and its components keep their own
licenses (see [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)). Pirates of the Caribbean: At World's End,
its code and its assets belong to their respective owners.

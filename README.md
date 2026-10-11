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
.\out\build\win-amd64-release\potcawerecomp.exe
```

- Game files are found in `assets\` next to `potcawerecomp.exe`, otherwise in the repository's `assets\`.
  Use `--game_data_root=<folder>` to point somewhere else.
- Saves, the profile and the shader cache go to `userdata\` next to `potcawerecomp.exe` (if that folder
  cannot be written, for example under Program Files, they go to `Documents\potcawe`). Logs go to `logs\`.

## Configuration

`potcawe.toml`, next to `potcawerecomp.exe`, holds the base settings. It is created with the defaults this
game needs if it is missing. Command-line options (`--name=value`) win over it, for example
`--gpu_backend=vulkan` or `--draw_resolution_scale_x=2 --draw_resolution_scale_y=2`.

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

## Credits

- [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk), whose runtime is derived from [Xenia](https://xenia.jp).
- [extract-xiso](https://github.com/XboxDev/extract-xiso), for extracting the game files.
- Developed by Gustavo Garcia with the help of Claude (Anthropic).

## License

MIT for this project's own code (see [`LICENSE`](LICENSE)). The ReXGlue SDK and its components keep their own
licenses (see [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)). Pirates of the Caribbean: At World's End,
its code and its assets belong to their respective owners.

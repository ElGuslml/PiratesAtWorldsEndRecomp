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
   either:
   - **the installer** (`...-setup.exe`): it installs for your user, without administrator rights, in
     `%LOCALAPPDATA%\Programs\PiratesAtWorldsEndRecomp`, with a Start menu shortcut (and a desktop one if you want).
     Uninstalling asks whether to delete your saves and settings, and the game files; or
   - **the zip**: extract it into a folder you can write to (not under `Program Files`).
2. Run `PiratesAtWorldsEnd Launcher.exe` (the installer can open it when it finishes). The first time, it asks for
   the ISO of your copy of the game, checks it, and extracts its files into the `assets` folder (see
   [Game files](#game-files)).
3. Choose your options and press Play. When you quit the game, the launcher comes back; Quit in the launcher
   closes it. For Steam, add the launcher: Steam sees the game running until you quit the launcher, and stopping
   it from Steam closes the game too. `PiratesAtWorldsEndRecomp.exe` still starts the game directly with the
   saved options.

Requirements:
- **System:** Windows 10 or 11, 64-bit, with a CPU with x86-64-v2 (SSE4.2 and POPCNT: practically any 64-bit
  CPU from 2009 on).
- **GPU:** one with a Vulkan driver for NativeXRenderer. NVIDIA DLAA needs an RTX GPU. The Xenos path also
  runs on Direct3D 12.
- **Runtime:** the [Microsoft Visual C++ Redistributable 2015–2022 (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe).

## Game files

Everything the game needs besides this project's code comes from your own copy of the game, an image (ISO) of
your own disc:
- **To play:** the launcher extracts it on its first run. Choose or drop the ISO (a full Redump image or a trimmed
  one); it checks that it is this game and this version (the USA disc, `default.xex` with CRC32 `A31B1069`), then
  copies its files into `assets` next to `PiratesAtWorldsEndRecomp.exe`. The ISO is not needed afterwards.
  `PiratesAtWorldsEnd Launcher.exe --launcher_first_run` installs them again.
- **To build:** extract the disc's content into the repository's `assets` folder, so that `assets/default.xex`
  exists, for example with [extract-xiso](https://github.com/XboxDev/extract-xiso/releases).

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
- **Quitting from the title screen:** B on "PRESS START" asks whether to quit the game; A quits, B cancels
  ([`docs/quit-prompt.md`](docs/quit-prompt.md)).
- **Achievements:** the game's 30 achievements unlock as on the console. A card at the top of the screen shows each
  one as it unlocks (its icon, name, description and gamerscore), F7 lists them all with their unlock dates, and the
  launcher's Achievements page shows them too. They are saved in `userdata\achievements\`
  ([`docs/achievements.md`](docs/achievements.md)).
- **On the Xenos path:**
  - no stutter the first time an effect appears ([`docs/stutters.md`](docs/stutters.md));
  - no flat-colored objects in Port Royal under GPU load
    ([`docs/flat-colored-objects.md`](docs/flat-colored-objects.md));
  - less GPU time at 2× and 3× ([`docs/gpu-load.md`](docs/gpu-load.md));
  - the sun's glare follows the sun ([`docs/sun-glow.md`](docs/sun-glow.md)).

## Configuration

**PiratesAtWorldsEnd Launcher** works with mouse, keyboard or any controller (DualSense, DualShock,
Xbox, Switch Pro...). Its pages:
- **Play:** starts the game, with a summary of the current settings.
- **Settings → Video:** monitor, display (fullscreen, borderless or window), window size, image format (fill
  the screen, or 16:9 with bars) and internal resolution (NativeXRenderer's, or the Xenos path's scale).
- **Settings → Graphics:** renderer (NativeXRenderer or Xenos), frame rate (30 or 60 FPS), antialiasing,
  anisotropic filtering, shadow quality with NativeXRenderer (Original, High or Ultra: the characters' shadow maps at
  the internal resolution, or at least 2048² or 4096²; Ultra takes about 250 MB more video memory), and the graphics
  API with Xenos.
- **Controls → Keyboard:** playing with keyboard and mouse, a primary and a secondary key for each control of
  the Xbox 360 controller (Shift, Ctrl or Alt combinations and mouse buttons included), and the mouse for the
  camera with its sensitivity.
- **Controls → Gamepad:** the controllers connected, and which of their buttons presses each button of the
  Xbox 360 controller (A, B, X, Y, the bumpers, the triggers, the stick clicks, Start, Back, Guide). Each type of
  controller (Xbox, PlayStation, Switch...) keeps its own layout.
- **Achievements:** the game's achievements with their icons, which ones you have unlocked and when, the totals and
  your gamerscore; filtered by all, unlocked or locked. The game writes their list the first time it runs.
- **Updates:** the installed version, and a newer release from this repository's
  [Releases](https://github.com/ElGuslml/PiratesAtWorldsEndRecomp/releases) with its notes. "Update now" downloads it,
  checks its SHA-256, replaces the program's files and restarts the launcher; your game files (`assets\`), saves and
  achievements (`userdata\`) and settings (`.toml`) are never touched. The launcher looks for a new version each time
  it opens unless you turn that off. Pre-releases are not offered.

Every change is saved at once. With a controller: LB/RB change the page, A selects, B goes back to the
sidebar, Start plays. The launcher starts in fullscreen (always on the Steam Deck and in Steam Big Picture); F11 or
Alt+Enter switch it to a window, which scales with its size. If the game closes with an error, the launcher says
so when it comes back, with a link to the logs.

| File (next to `PiratesAtWorldsEndRecomp.exe`) | What it holds |
|---|---|
| `potcawe.toml` | Base settings. Created with the defaults this game needs if it is missing. |
| `potcawe_options.toml` | What PiratesAtWorldsEnd Launcher saves. Loaded after `potcawe.toml`. |

Command-line options (`--name=value`, for example in Steam's launch options) win over both files. For
example:
- `--fullscreen=false --window_width=1920 --window_height=1080`;
- `--potcawe_60fps=true`;
- `--native_x_renderer=false` (the Xenos path);
- `--native_x_renderer_antialiasing=dlaa`;
- `--quit_prompt=false` (B on the title screen does not ask to quit).

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
- **Configuring** applies NativeXRenderer's DiligentCore patches; `thirdparty/rexglue-sdk` is already the SDK
  with this project's changes (see below).
- **The first build** translates `default.xex` into C++ (`generated/`) and builds the SDK: it takes a while.
  If it stops with "precompiled header ... has been modified", run the build command again.

## How the SDK is modified

`thirdparty/rexglue-sdk` is [ElGuslml/rexglue-sdk](https://github.com/ElGuslml/rexglue-sdk), branch `recomp`: the
official ReXGlue SDK's nightly of 2026-10-02 (v0.10.0 and 24 later commits) with the changes this project, GRAWRecomp
(Ghost Recon Advanced Warfighter), KameoRecomp (Kameo: Elements of Power) and
[NativeXRenderer](native_x_renderer/README.md) need, one commit each. None of the general fixes is specific to one
game, and the options they add are off by default; the ones for NativeXRenderer change nothing while it is off. The
branch's `RECOMP.md` lists them, and each commit message explains its change. Until 2026-10-21 they were patch series
applied at configure time (`patches/` here and NativeXRenderer's `sdk_patches/`).

A checkout made before that has the old submodule with the patches applied over it; update it with
`git submodule sync` and `git submodule update --init --recursive --force`, then configure again.

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

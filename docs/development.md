# Building and developing PiratesAtWorldsEndRecomp

PiratesAtWorldsEndRecomp is a static recompilation of **Pirates of the Caribbean: At World's End** (Xbox 360, 2007)
to native Windows, built on the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk). The game's PowerPC code is
translated to C++ and compiled. It is drawn by [NativeXRenderer](https://github.com/ElGuslml/NativeXRenderer), a
native renderer on Vulkan that draws the game's Direct3D calls on the PC's GPU instead of emulating the Xbox 360's;
ReXGlue's emulation of that GPU, Xenos (derived from [Xenia](https://xenia.jp)), stays available as an alternative.
The other notes in [`docs/`](README.md) describe each problem found and what was done about it.

## Requirements

- Git;
- [CMake](https://cmake.org/) 3.25 or newer and [Ninja](https://ninja-build.org/);
- [LLVM/Clang](https://github.com/llvm/llvm-project/releases): tested with Clang 23, and MSVC alone does not build
  this;
- the Windows SDK;
- Python 3;
- your own copy of the game: extract the disc's content into the repository's `assets` folder, so that
  `assets/default.xex` exists, for example with [extract-xiso](https://github.com/XboxDev/extract-xiso/releases).
  The expected executable is the USA disc's, CRC32 `A31B1069`; `PiratesAtWorldsEndRecomp.exe` checks it at startup
  and warns if yours is a different version. See [`assets/README.md`](../assets/README.md).

## Building

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
- **Configuring** applies NativeXRenderer's DiligentCore patches; `thirdparty/rexglue-sdk` is already the SDK with
  this project's changes (see below).
- **The first build** translates `default.xex` into C++ (`generated/`) and builds the SDK: it takes a while. If it
  stops with "precompiled header ... has been modified", run the build command again.

## Renderers

**NativeXRenderer**, the default:
- the game drawn at the screen's resolution or a lower internal resolution, wide screens (21:9, 16:10) filled by
  widening the game's 3D view, with the HUD, menus and videos kept at 16:9 in the middle;
- its own antialiasing: SMAA (the default), FXAA, or DLAA with NVIDIA DLSS on RTX GPUs;
- a persistent shader cache: from the second run, nothing waits for shaders to compile in places already seen;
- if it cannot start, the game falls back to the Xenos path.

**The Xenos path** (launcher: Renderer → Xenos): ReXGlue's emulation of the Xbox 360 GPU, on Direct3D 12 or Vulkan,
at a whole multiple of 1280×720 (2×, 3×), with the game's 2× MSAA. Wider screens get black bars. Its fixes: no
stutter the first time an effect appears ([`stutters.md`](stutters.md)), no flat-colored objects in Port Royal under
GPU load ([`flat-colored-objects.md`](flat-colored-objects.md)), less GPU time at 2× and 3× ([`gpu-load.md`](gpu-load.md)),
the sun's glare following the sun ([`sun-glow.md`](sun-glow.md)).

Fixes for both: the 60 FPS option ([`frame-rate.md`](frame-rate.md)), lower CPU use ([`cpu-usage.md`](cpu-usage.md)),
script errors that no longer close the game ([`script-errors.md`](script-errors.md)), quitting from the title screen
([`quit-prompt.md`](quit-prompt.md)) and the achievements ([`achievements.md`](achievements.md)).

## Configuration

| File (next to `PiratesAtWorldsEndRecomp.exe`) | What it holds |
|---|---|
| `potcawe.toml` | Base settings. Created with the defaults this game needs if it is missing. |
| `potcawe_options.toml` | What PiratesAtWorldsEnd Launcher saves. Loaded after `potcawe.toml`. |

Command-line options (`--name=value`, for example in Steam's launch options) win over both files, for example
`--fullscreen=false --window_width=1920 --window_height=1080`, `--potcawe_60fps=true`, `--native_x_renderer=false`
(the Xenos path), `--native_x_renderer_antialiasing=dlaa`, `--quit_prompt=false` (B on the title screen does not ask
to quit).

- Saves, the profile, the achievements and the shader caches go to `userdata\` next to the executable; if that
  folder cannot be written, to `Documents\potcawe`. Logs go to `logs\`.
- Game files are read from `assets\` next to the executable; `--game_data_root=<folder>` points somewhere else.
  `PiratesAtWorldsEnd Launcher.exe --launcher_first_run` installs them again from an ISO.

## The launcher

PiratesAtWorldsEnd Launcher (SDL3 + Dear ImGui) is [RecompLauncher](https://github.com/ElGuslml/RecompLauncher)
(`recomp_launcher/`, a submodule: the launcher of every one of these recompilations, first made for this game) with
this game's profile in `launcher/` (`game.h`, `game.cpp`, `palette.h` and `art/`) and its resources in
`res/launcher.rc`. It starts in fullscreen when Steam asks for it (`SteamDeck=1`,
`SteamGamepadUI=1`: Big Picture); F11 or Alt+Enter switch it to a window. Its updater downloads a newer release from
this repository's Releases, checks its SHA-256 and replaces the program's files; the game files (`assets\`), saves
and achievements (`userdata\`) and settings (`.toml`) are never touched, and pre-releases are not offered.
`installer/potcawe.iss` is its Inno Setup installer.

## How the SDK is modified

`thirdparty/rexglue-sdk` is [ElGuslml/rexglue-sdk](https://github.com/ElGuslml/rexglue-sdk), branch `recomp`: the
official ReXGlue SDK's nightly of 2026-10-02 (v0.10.0 and 24 later commits) with the changes this project, GRAWRecomp
(Ghost Recon Advanced Warfighter), KameoRecomp (Kameo: Elements of Power) and
[NativeXRenderer](https://github.com/ElGuslml/NativeXRenderer) need, one commit each. None of the general fixes is
specific to one game, and the options they add are off by default; the ones for NativeXRenderer change nothing while
it is off. The branch's `RECOMP.md` lists them, and each commit message explains its change. Until 2026-10-21 they
were patch series applied at configure time (`patches/` here and NativeXRenderer's `sdk_patches/`); a checkout from
before that needs `git submodule sync` and `git submodule update --init --recursive --force`, then a new configure.

## More documentation

- [`native-x-renderer-potcawe.md`](native-x-renderer-potcawe.md): the game side of NativeXRenderer (its Direct3D
  map, engine behavior and measurements).
- [NativeXRenderer](https://github.com/ElGuslml/NativeXRenderer): the renderer's design, work plan and decision log.

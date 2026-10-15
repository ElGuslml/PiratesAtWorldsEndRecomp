# CLAUDE.md — POTCAWERecompVulkan

The development home of **NativeXRenderer**, a native renderer for Xbox 360 games recompiled with the ReXGlue SDK
(a PPC → C++ translator plus an Xbox 360 runtime derived from Xenia). NativeXRenderer hooks the game's XDK
Direct3D library and draws on the host through DiligentCore (Vulkan), instead of emulating the Xenos GPU.
The SDK's Xenos path stays as the fallback and the reference. It is developed on Pirates of the Caribbean:
At World's End (Xbox 360, Eurocom, 2007), and is meant to be reused by GRAWRecomp, KameoRecomp and later
projects.

Everything in the repository is written in **English**: code comments, log messages, documents, commit
messages, tool output. Personal notes such as local paths go in `CLAUDE.local.md`, which is gitignored.

## Origin and relation to POTCAWERecomp

Created on 2026-10-13 as a clone of POTCAWERecomp, branch `rt-to-texture` at `c6be8a3` (SDK patches
0001–0020), with its history. The remote `potcawe` is the local POTCAWERecomp, fetch-only. Game fixes
(hooks, launcher, settings) are made in POTCAWERecomp first and merged here with `git fetch potcawe` plus a
merge. POTCAWERecomp's `CLAUDE.md` and `local_docs/` hold the full history of the game work (CPU usage,
stutters, 60 FPS, flat-colored objects, GPU load, sun glare). The parts NativeXRenderer needs are summarized in
`docs/native-x-renderer-potcawe.md`.

The game, as inherited:
- **Executable:** `default.xex`, original name `PiratesXT_XE.exe`, title ID `425607D1`, CRC32 `A31B1069`,
  XDK 2.0.5426.
- **Rendering:** at a fixed 1280×720, 2× MSAA in two EDRAM tiles (predicated tiling), and 30 FPS from its
  present interval.
- **Game-side hooks:**
  - `src/gpu_wait.cpp`: the D3D library's waits sleep instead of spinning;
  - `src/framerate.cpp`: the 60 FPS option;
  - `src/msaa.cpp`: the Antialiasing option, plus the flare AA level.
- **Launcher:** `launcher/potcawe_launcher.cpp` and `src/options.cpp`.
- **Settings:** the defaults the game needs are in `src/startup.cpp` (`dxbc_switch = false`,
  `gpu_async_pipelines_no_frame_wait`, `execute_unclipped_draw_vs_on_cpu`,
  `clear_memory_page_state = false`…).

## Status

**Phase 0 (scaffolding) done, 2026-10-13.**
- Project created; game files and tools copied.
- The baseline build passes, and Gustavo confirmed it runs perfectly.
- Documents written.
- The library skeleton (`native_x_renderer/`, static library, linked, logs one line at startup) builds.
- Baseline of the Xenos path measured: nine `gpu_ab.ps1` runs (`local_docs/01-baseline.md`, the table in
  `plan.md`). At 2×2: MSAA about 14 ms of GPU time per frame (82.6 % at 60 FPS), without MSAA about
  8.5–9 ms. At 3×3: MSAA 33 FPS.

Phase 0 is committed (`0aacfea`, 2026-10-13) and pushed to `origin`, a private GitHub repository. This file
is tracked; `CLAUDE.local.md`, `tools/`, `local_docs/` and `assets/` stay gitignored, as inherited from
POTCAWERecomp.

**Phase 1 (map and observe) done, 2026-10-13.**
- **D3D map:** 39 entry points hooked in `src/native_x_renderer_potcawe.cpp`, with the XDK 2.0.5426 device layout.
- **Trace:** `--native_x_renderer_trace`, F9 dumps a frame.
- **Observer:** `--native_x_renderer_verify`, F10 verifies a frame, through SDK patch
  `native_x_renderer/sdk_patches/0001`.
- **Result:** 100 % of the draws rebuilt at the title screen, the main menu and the Port Royal dock, with
  and without 2× MSAA.
- **Open questions:** all answered in `docs/native-x-renderer-potcawe.md`.
- **Decisions:** D11–D13.
- **Notes:** `local_docs/02`–`04`.
- **Analysis tools** (tracked, in `native_x_renderer/tools/`, see its README): `ppc_index.py`, `pm4_consts.py`,
  `fn_summary.py`, `rs_setters.py`, `frame_trace.py`. Run `python native_x_renderer/tools/ppc_index.py build` after
  a codegen.

**Phase 2 (core renderer) done, 2026-10-13 to 2026-10-14** (go: D14; exit check in `plan.md`). With `--native_x_renderer=true`, or the
launcher's Renderer row on NativeXRenderer (D25; it forces Antialiasing Off for now):
- the SDK's command processor runs for synchronization only (`native_x_renderer/sdk_patches/0002`), and
  NativeXRenderer presents through its own DiligentCore (Vulkan) swap chain;
- render targets, clears, resolves and Present work;
- every draw goes through NativeXRenderer's draw path (`native_x_renderer/src/draw.cpp`): the SDK's translator, its
  `SharedMemory` and `PrimitiveProcessor` on DiligentCore buffers, DiligentCore pipelines;
- textures come from the SDK's `TextureCache` on DiligentCore (`native_x_renderer/src/guest_textures.cpp`, step
  7 done on 2026-10-14, D23): the language menu, the title screen and main menu (Bink backgrounds) and the
  Port Royal dock (HUD, character shadows) match the Xenos path;
- GPU time at the dock is 11.7–12.0 ms per frame at 1× with Antialiasing Off (the options' default; every
  Phase 2 run had AA Off), against 8.5–9 ms for the Xenos path at 2×2: too much (Phase 3). 60 FPS, no stalls
  since the DiligentCore patch for robust buffer access (D22).

Decisions D15–D28; notes `local_docs/05-phase2.md`. Bring-up aids: `--native_x_renderer_draw_log=N`, DiligentCore
messages in the log, the frame trace (F9), RenderDoc captures of the Vulkan device
(`tools/renderdoc/xn_inspect.py`, `xn_users.py`).

**Phase 3 (performance and the renderer's own features) done, 2026-10-14** (go: D29; scope and order: D31;
closed: D39). Results at the dock:
- GPU time 12.1 → 1.65 ms per frame at 1×:
  - render targets bound only when they change (D30);
  - guest memory pages copied only when their data changed (D35), which replaced the planned upload
    step A;
- the occlusion queries (the sun glare) measured on the host and answered to the sync-only command
  processor (`native_x_renderer/sdk_patches/0003`, D32);
- DXT1 kept compressed as BC1 (`native_x_renderer/diligent_patches/0002`): texture memory 276–318 →
  128–139 MB (D33);
- shader translation on two worker threads; a draw waits by being skipped (D34);
- the guest's display gamma ramp applied when presenting (`sdk_patches/0004`, D36);
- the render resolution follows the screen at any scale, with no selector (D37, D38; `sdk_patches/0005`
  for the translator). The launcher shows the resulting resolution with NativeXRenderer;
- the A/B against the Xenos path (`tools/gpu_ab.ps1`; the table in `plan.md`): at the same resolution
  without antialiasing NativeXRenderer uses 38–56 % less of the GPU; at 3× it holds 60 FPS where the Xenos
  path with 2× MSAA drops to 33.

Decisions D29–D39; notes `local_docs/06-phase3-performance.md`.

**Phase 4 (POTCAWE polish) done, 2026-10-14 to 2026-10-15** (D39, D40; closed: D57). First, in this order:
1. NativeXRenderer's own antialiasing: done (D41). SMAA (default) or FXAA on the copy of the finished scene,
   before the HUD; the launcher's Antialiasing row chooses it with NativeXRenderer;
2. a persistent pipeline cache: done (D42). The shaders, translations and pipelines used are kept per
   title in the runtime's cache directory and prepared at the next start; from the second run no draw waits
   at the dock's load;
3. any aspect ratio without bars (ultrawide, 16:10, the Steam Deck): done (D43). The output is filled
   with a scale per axis, the game's 16:9 cameras are widened (`src/aspect.cpp`, culling included), and the
   HUD, menus and videos are drawn in a centered 16:9 box.

Also done: anisotropic filtering (D44). NativeXRenderer applies the SDK's `anisotropic_override` as the Xenos path
does, and POTCAWE defaults to 16x with a launcher row (made in POTCAWERecomp, `f3b67db`, and merged). The
Vulkan pipeline cache is saved on a thread of its own (D45): its saves were the 30–80 ms stutters every few
minutes.

Notes: `local_docs/07-phase4.md`.

Moved to the plan's "Later" when Phase 4 closed (D57):
- every level compared side by side with the Xenos path;
- the known limits from Phase 2;
- other levels and the cutscenes at 21:9;
- HUD elements anchored to the screen's edges.

Gustavo's first long session (2026-10-15) found the journal showing the scene through its pages: a
resolve from a `2_10_10_10_FLOAT` target now takes the guest format's range (D50). His second (54 minutes,
levels 1 to 3) ran at 60 FPS with hitches only at new content. It led to:
- resolves into cube textures (D51);
- freeing render targets and resolved textures unused for a minute, and saving the pipeline cache at most
  every 5 minutes (D52).

The launcher with NativeXRenderer now chooses the image format (fill a wide screen, or 16:9 with bars) and the
internal resolution (automatic, or a lower height), in fullscreen and in a window (D54). Its Save keeps the
window open, and its antialiasing row no longer offers the DLSS modes (D53).

The session's device memory growth is explained (D55):
- the figure was DiligentCore's pages;
- the use is the guest textures at the SDK cache's limit, the targets D52 frees, the fixed buffers, and
  textures idle pipelines kept alive. Those pipelines now let go of their textures after 10 s;
- the log shows the memory in use every 600 frames.

His third session (30 minutes, run 288) showed the memory flat within a level. It ended when the game
closed itself: a Lua script error that the recompiled `longjmp` turned into `exit()`. The fix,
`setjmp_address` / `longjmp_address` in `potcawe_functions.toml`, was made in POTCAWERecomp and merged
(`docs/script-errors.md`). His fourth (64 minutes, run 291) completed the hour-long session:
- no problem in play;
- four script errors caught by the game, which it survived;
- the memory in use bounded at 1.64–1.77 GB;
- no message from the renderer.

NativeXRenderer is now POTCAWE's default renderer (D56):
- `native_x_renderer = true` in `src/startup.cpp`, and the launcher's default;
- the Xenos path stays in the launcher, and draws if NativeXRenderer cannot start.

Anchoring HUD elements to the screen's edges is left for a later phase (`plan.md`, "Later").

**Phase 5 (temporal techniques) started on 2026-10-14, with DLSS, before Phase 4 closes** (D46, at Gustavo's
request): DLSS through DiligentCore's SuperResolution module (NGX on Vulkan); found and reported with
`native_x_renderer_antialiasing = dlaa`. DLAA works (D47): the main 3D view jittered through its viewport, the camera's
motion vectors (POTCAWE's depth is reversed), DLSS before the game's post-processing, a launcher choice;
+1.2 ms at 2×. Objects that move on their own have motion vectors of their own (D48): the draws are matched
across frames, and those that moved are drawn again with their vertex shader run on both frames' constants.
No measurable GPU cost; the render thread spends 0.65 ms more. The DLSS modes (Quality, Balanced,
Performance, Ultra Performance) draw the main 3D view at NGX's input resolution and upscale it before the
post-processing (D49, DiligentCore patch 0005). At 3×: DLAA 60.5 % of the GPU, Quality 51.4 %, Ultra
Performance 46.3 %, SMAA 45.9 %. The DLSS modes are experimental: the launcher offers only DLAA, and
`potcawe_options.toml` can still name a mode (D53). Notes: `local_docs/08-phase5.md`.

Phase 5 is temporal upscaling and frame generation (motion vectors, DLSS / DLAA, FSR 3), and Phase 6 the
second game and the library's extraction (D40). Follow `native_x_renderer/docs/plan.md` and tick its boxes as
work is done.

## Read first

- `native_x_renderer/README.md`: what NativeXRenderer is, the layers, the document index.
- `native_x_renderer/docs/plan.md`: the work plan, the baseline to beat, the test protocol, the risks.
- `native_x_renderer/docs/architecture.md`: the design. Sections marked "to confirm in Phase N" are not settled
  yet.
- `native_x_renderer/docs/xdk-d3d-reference.md`: the XDK D3D library across games, and how to find its entry
  points.
- `native_x_renderer/docs/decisions.md`: why things are the way they are. Read it before changing course.
- `docs/native-x-renderer-potcawe.md`: POTCAWE's D3D map, device offsets and engine behavior so far.
- Hell's Gate's design documents, kept for reference in `local_docs/reference/hells-gate/` (gitignored).
  The source is https://github.com/florinp93/hells-gate-recomp.

## Layout

| Path | What it is |
|---|---|
| `native_x_renderer/` | The renderer library: `CMakeLists.txt` (target `native_x_renderer`), `include/native_x_renderer/`, `src/`, `docs/`, `tools/` (analysis scripts for bringing up a game), `sdk_patches/`. No game-specific code. |
| `docs/` | Published game notes (one per problem), plus `native-x-renderer-potcawe.md` (the game side of NativeXRenderer). |
| `src/` | The game: startup, launcher options, game hooks. The NativeXRenderer hook file and profile for POTCAWE go here (`src/native_x_renderer_potcawe.cpp`, Phase 1). |
| `thirdparty/rexglue-sdk` | Official ReXGlue SDK, submodule pinned to v0.10.0 (`c94f5eb`), with `patches/` applied at configure time. |
| `thirdparty/diligent-core` | DiligentCore (Vulkan backend only), a submodule pinned to master `074060c`, with `native_x_renderer/diligent_patches/` applied at configure time. |
| `patches/` | The shared SDK series (identical to GRAWRecomp and KameoRecomp). |
| `native_x_renderer/sdk_patches/` | NativeXRenderer's own SDK patches (D10/D11), applied after `patches/`. |
| `native_x_renderer/diligent_patches/` | NativeXRenderer's DiligentCore patches (D22), applied to `thirdparty/diligent-core` at configure time. |
| `potcawe_manifest.toml`, `potcawe_functions.toml`, `potcawe_functions_auto.toml` | Codegen input. |
| `launcher/`, `res/`, `cmake/` | As in POTCAWERecomp. |
| `assets/` | The game files: a real copy, 6.4 GB, gitignored. |
| `tools/` | Personal, gitignored, copied from POTCAWERecomp with paths repointed. Includes `game.ps1`, `level2.ps1`, `gpu_ab.ps1`, `renderdoc/`, `threadcpu.ps1`, `xdkver.mjs` and `peek.mjs`. |
| `local_docs/` | Gitignored detailed notes, one numbered file per topic, plus `reference/`. |

## Building (Windows)

```powershell
cmake --preset win-amd64-relwithdebinfo        # applies patches/ to the submodule, configures the SDK too
cmake --build --preset win-amd64-relwithdebinfo
.\out\build\win-amd64-relwithdebinfo\PiratesAtWorldsEndRecomp.exe   # or "PiratesAtWorldsEnd Launcher.exe"
```

- **Toolchain:** LLVM/Clang, Ninja and the Windows SDK. Python 3 at configure time.
- **The first build** after a codegen may fail with "precompiled header ... has been modified": build again.
- **After a codegen**, check the switch tables and the holes as in POTCAWERecomp:
  - `node tools/check_switch.mjs generated/default tools/image.bin`
  - `python tools/find_holes.py --image tools/image.bin`

## Rules for NativeXRenderer

1. **`native_x_renderer/` contains no game-specific code or addresses.** Game addresses, device offsets and
   engine quirks live in the game's sources and documents. A callback the game needs is added to the core
   in a general form.
2. **Hook definitions (`REX_HOOK_RAW` over D3D entry points) are compiled into the executable**, never into
   the static library. The generated functions are weak, and a strong definition in an archive member is
   not guaranteed to be pulled in.
3. **No code from Hell's Gate.** Its repository has no license (D7 in `decisions.md`). Its documents inform
   the design, and every idea taken from it is credited in `native_x_renderer/docs/credits.md`.
4. **The Xenos path must keep working** with `native_x_renderer` off: every hook is a pass-through then. Compare
   every visual result with the Xenos path at the same spot.
5. **SDK changes that only NativeXRenderer needs** go into `native_x_renderer/sdk_patches/`, applied after the
   shared series (D10, proposed). The shared `patches/` series stays identical to GRAWRecomp's and
   KameoRecomp's.
6. **Documentation is part of the work.** When a phase step is done, tick it in `plan.md`. When a design
   point is settled, update `architecture.md`. Record decisions in `decisions.md`, with a new entry and
   never by rewriting an old one. Game facts with their evidence go into `docs/native-x-renderer-potcawe.md`.
   Facts that are true for every game go into `xdk-d3d-reference.md`. Keep a numbered `local_docs/` file
   per topic for runs and logs, as in POTCAWERecomp.
7. **The trace mode and the observer are library features** (off by default, free when off), not debug
   instrumentation. Ad-hoc instrumentation still never goes into `src/`, `native_x_renderer/` or `patches/`.

## Rules inherited from POTCAWERecomp

1. **Do not remove the SDK fixes** (`patches/`): they are shared with GRAWRecomp and KameoRecomp.
2. **Debug instrumentation never goes into `patches/` or `src/`** (see rule 7 above for the library's
   own tools).
3. **The settings this game needs are built into `src/startup.cpp`.**
4. **The game is tested by hand by Gustavo.** Ask for concrete, short tests, group several questions into
   one run, and check whether the existing logs already answer the question before asking for another run.
   Prefer the automated drivers (`tools/game.ps1`, `tools/level2.ps1`, `tools/gpu_ab.ps1`) for anything
   that does not need a human.
5. **Never revert a file with uncommitted work** (`git checkout -- file`). Undo edits by hand.

## Changing the SDK

As in POTCAWERecomp:
1. Create a branch at `c94f5eb` in `thirdparty/rexglue-sdk`, `git am` the series, and commit the change.
2. `git format-patch c94f5eb..HEAD -o ../../patches` (delete the old files first).
3. Add a row to the README's patch table.
4. Reconfigure, after `git -C thirdparty/rexglue-sdk checkout -- .`.

A general fix also goes to POTCAWERecomp, GRAWRecomp and KameoRecomp. NativeXRenderer-only SDK changes follow
rule 5 above:
1. Use the submodule's branch `native-x-renderer`: the shared series with `git am`, then NativeXRenderer's patches.
   A `git worktree` in the scratchpad keeps the submodule's working tree untouched.
2. Commit the change.
3. Export only NativeXRenderer's commits:
   `git format-patch <last shared commit>..HEAD -o <repo>/native_x_renderer/sdk_patches`.
4. Reconfigure as above. `cmake/apply_sdk_patches.cmake` applies `native_x_renderer/sdk_patches/` after
   `patches/`.

## Changing DiligentCore

The submodule stays at its pinned commit; NativeXRenderer's changes are `native_x_renderer/diligent_patches/` (D22),
applied by `native_x_renderer/cmake/apply_diligent_patches.cmake` when configuring:
1. Use the submodule's branch `native-x-renderer` (pinned commit plus the series), checked out in a
   `git worktree` in the scratchpad.
2. Commit the change there.
3. `git format-patch <pinned commit>..HEAD -o <repo>/native_x_renderer/diligent_patches` (delete the old files
   first).
4. `git -C thirdparty/diligent-core checkout -- .`, then configure again.

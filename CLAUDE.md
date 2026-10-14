# CLAUDE.md — POTCAWERecompVulkan

The development home of **XenosNative**, a native renderer for Xbox 360 games recompiled with the ReXGlue SDK
(a PPC → C++ translator plus an Xbox 360 runtime derived from Xenia). XenosNative hooks the game's XDK
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
stutters, 60 FPS, flat-colored objects, GPU load, sun glare). The parts XenosNative needs are summarized in
`docs/xenos-native-potcawe.md`.

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
- The library skeleton (`xenos_native/`, static library, linked, logs one line at startup) builds.
- Baseline of the Xenos path measured: nine `gpu_ab.ps1` runs (`local_docs/01-baseline.md`, the table in
  `plan.md`). At 2×2: MSAA about 14 ms of GPU time per frame (82.6 % at 60 FPS), without MSAA about
  8.5–9 ms. At 3×3: MSAA 33 FPS.

Phase 0 is committed (`0aacfea`, 2026-10-13) and pushed to `origin`, a private GitHub repository. This file
is tracked; `CLAUDE.local.md`, `tools/`, `local_docs/` and `assets/` stay gitignored, as inherited from
POTCAWERecomp.

**Phase 1 (map and observe) done, 2026-10-13.**
- **D3D map:** 39 entry points hooked in `src/xenos_native_potcawe.cpp`, with the XDK 2.0.5426 device layout.
- **Trace:** `--xenos_native_trace`, F9 dumps a frame.
- **Observer:** `--xenos_native_verify`, F10 verifies a frame, through SDK patch
  `xenos_native/sdk_patches/0001`.
- **Result:** 100 % of the draws rebuilt at the title screen, the main menu and the Port Royal dock, with
  and without 2× MSAA.
- **Open questions:** all answered in `docs/xenos-native-potcawe.md`.
- **Decisions:** D11–D13.
- **Notes:** `local_docs/02`–`04`.
- **Analysis tools** (tracked, in `xenos_native/tools/`, see its README): `ppc_index.py`, `pm4_consts.py`,
  `fn_summary.py`, `rs_setters.py`, `xn_trace.py`. Run `python xenos_native/tools/ppc_index.py build` after
  a codegen.

**Phase 2 (core renderer) done, 2026-10-13 to 2026-10-14** (go: D14; exit check in `plan.md`). With `--xenos_native=true`, or the
launcher's Renderer row on XenosNative (D25; it forces Antialiasing Off for now):
- the SDK's command processor runs for synchronization only (`xenos_native/sdk_patches/0002`), and
  XenosNative presents through its own DiligentCore (Vulkan) swap chain;
- render targets, clears, resolves and Present work;
- every draw goes through XenosNative's draw path (`xenos_native/src/draw.cpp`): the SDK's translator, its
  `SharedMemory` and `PrimitiveProcessor` on DiligentCore buffers, DiligentCore pipelines;
- textures come from the SDK's `TextureCache` on DiligentCore (`xenos_native/src/guest_textures.cpp`, step
  7 done on 2026-10-14, D23): the language menu, the title screen and main menu (Bink backgrounds) and the
  Port Royal dock (HUD, character shadows) match the Xenos path;
- GPU time at the dock is 11.7–12.0 ms per frame at 1× with Antialiasing Off (the options' default; every
  Phase 2 run had AA Off), against 8.5–9 ms for the Xenos path at 2×2: too much (Phase 3). 60 FPS, no stalls
  since the DiligentCore patch for robust buffer access (D22).

Decisions D15–D30; notes `local_docs/05-phase2.md`. Bring-up aids: `--xenos_native_draw_log=N`, DiligentCore
messages in the log, the frame trace (F9), RenderDoc captures of the Vulkan device
(`tools/renderdoc/xn_inspect.py`, `xn_users.py`).

**Phase 3 (parity in POTCAWE) in progress, since 2026-10-14** (go: D29). The agreed order: GPU time first (the
town side of the dock costs 12 ms, the Xenos path ~5 ms), then the occlusion queries (the sun glare), then
resolution, MSAA and the rest of the game. Follow `xenos_native/docs/plan.md` and tick its boxes as work is
done.

## Read first

- `xenos_native/README.md`: what XenosNative is, the layers, the document index.
- `xenos_native/docs/plan.md`: the work plan, the baseline to beat, the test protocol, the risks.
- `xenos_native/docs/architecture.md`: the design. Sections marked "to confirm in Phase N" are not settled
  yet.
- `xenos_native/docs/xdk-d3d-reference.md`: the XDK D3D library across games, and how to find its entry
  points.
- `xenos_native/docs/decisions.md`: why things are the way they are. Read it before changing course.
- `docs/xenos-native-potcawe.md`: POTCAWE's D3D map, device offsets and engine behavior so far.
- Hell's Gate's design documents, kept for reference in `local_docs/reference/hells-gate/` (gitignored).
  The source is https://github.com/florinp93/hells-gate-recomp.

## Layout

| Path | What it is |
|---|---|
| `xenos_native/` | The renderer library: `CMakeLists.txt` (target `xenos_native`), `include/xenos_native/`, `src/`, `docs/`, `tools/` (analysis scripts for bringing up a game), `sdk_patches/`. No game-specific code. |
| `docs/` | Published game notes (one per problem), plus `xenos-native-potcawe.md` (the game side of XenosNative). |
| `src/` | The game: startup, launcher options, game hooks. The XenosNative hook file and profile for POTCAWE go here (`src/xenos_native_potcawe.cpp`, Phase 1). |
| `thirdparty/rexglue-sdk` | Official ReXGlue SDK, submodule pinned to v0.10.0 (`c94f5eb`), with `patches/` applied at configure time. |
| `thirdparty/diligent-core` | DiligentCore (Vulkan backend only), a submodule pinned to master `074060c`, with `xenos_native/diligent_patches/` applied at configure time. |
| `patches/` | The shared SDK series (identical to GRAWRecomp and KameoRecomp). |
| `xenos_native/sdk_patches/` | XenosNative's own SDK patches (D10/D11), applied after `patches/`. |
| `xenos_native/diligent_patches/` | XenosNative's DiligentCore patches (D22), applied to `thirdparty/diligent-core` at configure time. |
| `potcawe_manifest.toml`, `potcawe_functions.toml`, `potcawe_functions_auto.toml` | Codegen input. |
| `launcher/`, `res/`, `cmake/` | As in POTCAWERecomp. |
| `assets/` | The game files: a real copy, 6.4 GB, gitignored. |
| `tools/` | Personal, gitignored, copied from POTCAWERecomp with paths repointed. Includes `game.ps1`, `level2.ps1`, `gpu_ab.ps1`, `renderdoc/`, `threadcpu.ps1`, `xdkver.mjs` and `peek.mjs`. |
| `local_docs/` | Gitignored detailed notes, one numbered file per topic, plus `reference/`. |

## Building (Windows)

```powershell
cmake --preset win-amd64-relwithdebinfo        # applies patches/ to the submodule, configures the SDK too
cmake --build --preset win-amd64-relwithdebinfo
.\out\build\win-amd64-relwithdebinfo\potcawerecomp.exe
```

- **Toolchain:** LLVM/Clang, Ninja and the Windows SDK. Python 3 at configure time.
- **The first build** after a codegen may fail with "precompiled header ... has been modified": build again.
- **After a codegen**, check the switch tables and the holes as in POTCAWERecomp:
  - `node tools/check_switch.mjs generated/default tools/image.bin`
  - `python tools/find_holes.py --image tools/image.bin`

## Rules for XenosNative

1. **`xenos_native/` contains no game-specific code or addresses.** Game addresses, device offsets and
   engine quirks live in the game's sources and documents. A callback the game needs is added to the core
   in a general form.
2. **Hook definitions (`REX_HOOK_RAW` over D3D entry points) are compiled into the executable**, never into
   the static library. The generated functions are weak, and a strong definition in an archive member is
   not guaranteed to be pulled in.
3. **No code from Hell's Gate.** Its repository has no license (D7 in `decisions.md`). Its documents inform
   the design, and every idea taken from it is credited in `xenos_native/docs/credits.md`.
4. **The Xenos path must keep working** with `xenos_native` off: every hook is a pass-through then. Compare
   every visual result with the Xenos path at the same spot.
5. **SDK changes that only XenosNative needs** go into `xenos_native/sdk_patches/`, applied after the
   shared series (D10, proposed). The shared `patches/` series stays identical to GRAWRecomp's and
   KameoRecomp's.
6. **Documentation is part of the work.** When a phase step is done, tick it in `plan.md`. When a design
   point is settled, update `architecture.md`. Record decisions in `decisions.md`, with a new entry and
   never by rewriting an old one. Game facts with their evidence go into `docs/xenos-native-potcawe.md`.
   Facts that are true for every game go into `xdk-d3d-reference.md`. Keep a numbered `local_docs/` file
   per topic for runs and logs, as in POTCAWERecomp.
7. **The trace mode and the observer are library features** (off by default, free when off), not debug
   instrumentation. Ad-hoc instrumentation still never goes into `src/`, `xenos_native/` or `patches/`.

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

A general fix also goes to POTCAWERecomp, GRAWRecomp and KameoRecomp. XenosNative-only SDK changes follow
rule 5 above:
1. Use the submodule's branch `xenos-native`: the shared series with `git am`, then XenosNative's patches.
   A `git worktree` in the scratchpad keeps the submodule's working tree untouched.
2. Commit the change.
3. Export only XenosNative's commits:
   `git format-patch <last shared commit>..HEAD -o <repo>/xenos_native/sdk_patches`.
4. Reconfigure as above. `cmake/apply_sdk_patches.cmake` applies `xenos_native/sdk_patches/` after
   `patches/`.

## Changing DiligentCore

The submodule stays at its pinned commit; XenosNative's changes are `xenos_native/diligent_patches/` (D22),
applied by `xenos_native/cmake/apply_diligent_patches.cmake` when configuring:
1. Use the submodule's branch `xenos-native` (pinned commit plus the series), checked out in a
   `git worktree` in the scratchpad.
2. Commit the change there.
3. `git format-patch <pinned commit>..HEAD -o <repo>/xenos_native/diligent_patches` (delete the old files
   first).
4. `git -C thirdparty/diligent-core checkout -- .`, then configure again.

# XenosNative in POTCAWE

The game side of XenosNative for Pirates of the Caribbean: At World's End: what is known about the game's
Direct3D layer and its rendering, what is still to find, and how it is measured. For the renderer itself
see [`../xenos_native/README.md`](../xenos_native/README.md). The work plan is
[`../xenos_native/docs/plan.md`](../xenos_native/docs/plan.md).

Sources:
- POTCAWERecomp's investigation notes (`local_docs/01`–`06` in that repository, gitignored there);
- its hooks (`src/gpu_wait.cpp`, `src/framerate.cpp`, `src/msaa.cpp`);
- the RenderDoc analysis of the Port Royal dock.

## The executable

| | |
|---|---|
| File | `default.xex` (original name `PiratesXT_XE.exe`), title ID `425607D1`, CRC32 `A31B1069` |
| XDK | 2.0.5426.0 (linked against; minimum 2.0.4552.0) |
| D3D library range | `0x822A0000`–`0x822CFFFF` |
| Engine renderer | Around `0x823Bxxxx`–`0x823Fxxxx` and `0x824xxxxx` (surface creation, present wrapper, queries) |
| Videos | Bink (`.bik`) |

## Known D3D functions

Found while working on POTCAWERecomp, before XenosNative. Status: **known** means the role was verified
(by a hook that works, or by the packets it writes); **likely** means the role was read from the code.

| Address | Function | Status | Evidence |
|---|---|---|---|
| `sub_822B58B0` | Swap (`r4 = 0`: Synchronize) | known | 60 FPS hook at `0x822B5968` reads the interval it encodes |
| `sub_822B5D50` | Synchronize (alias of Swap with no front buffer) | likely | |
| `sub_822B5D58` | Present: VdSwap, then Swap; waits for pending swaps | known | CPU usage investigation |
| `sub_822B5718` | Swap callback, run at vblank: flips the front buffer | likely | |
| `sub_822B55A8` | Vblank interrupt handler | likely | |
| `sub_822B3B40` | SetRenderState_PresentInterval (`device+13444`) | known | 60 FPS investigation |
| `sub_822B6338` | Device reset (re-applies the present interval) | likely | |
| `sub_822AE600` | Wait for ring space | known | GPU wait hooks |
| `sub_822ADA00` | Wait for a ring range | likely | |
| `sub_822AD968` | Wait for a fence | likely | |
| `sub_822C3388`, `sub_822C3628` | Wait for write-back slots | likely | |
| `sub_822AF458` | Wait begin (records the start) | known | `PotcaweGpuWaitBeginHook` |
| `sub_822AF608` | Wait iteration and hang check | known | `PotcaweGpuWaitSpinHook` |
| `sub_822AF488` | Adds wait time to the device counters | likely | |
| `sub_822B1ED8` | GPU hang handler | likely | |
| `sub_822AE6C8` | Insert a GPU callback packet `(device, ring, 0, callback, argument)` | known | Swap's descriptor path |
| `sub_822BA590` | D3DQuery Issue | known | sun flare investigation (patch 0020) |
| `sub_822C4848` | Emits EVENT_WRITE_ZPD (begin: `block+32`, end: `block+0`) | known | sun flare investigation |
| `sub_822BA818` | D3DQuery GetData (`D3DGETDATA_FLUSH`) | known | sun flare investigation |
| `sub_822ABE40` | XGSetSurfaceHeader | known | Antialiasing option |
| `sub_822AA730` | Surface tile size | known | Antialiasing option |

Still to find in Phase 1:
- the draws;
- Clear, Resolve, BeginTiling and EndTiling;
- SetRenderTarget and SetDepthStencilSurface;
- the shader, declaration, stream, index, texture and sampler setters;
- the constant setters and constant loads;
- the dirty-state flush;
- command buffer reservation, InsertFence and BlockOnFence.

See the [entry point table](../xenos_native/docs/xdk-d3d-reference.md#entry-points).

## Known device offsets (XDK 5426)

| Offset | Field |
|---|---|
| `+10768` | Pointer read by the GPU timing code (with `mftb`) |
| `+10772` | Pointer to the swap/vblank block: `+4` Synchronize wait flag, `+16`/`+20` last flip |
| `+10780` | Ring write pointer |
| `+11196`, `+11200` | Write-back slots |
| `+11716` | D3DRS_PRESENTIMMEDIATETHRESHOLD |
| `+13444` | D3DRS_PRESENTINTERVAL (the game sets 2 = 30 FPS) |
| `+16396` / `+16404` / `+16408` / `+16416` | Vblank count / last flip vblank / swap counters |
| `+21400` | Refresh rate (from VdQueryVideoMode) |
| `+21440`, `+21448` | Wait time counters |

Still to find: the register shadow, the float/bool/loop constants, the fetch constants, the current shaders
and surfaces, and the dirty masks.

## Engine functions that matter for rendering

| Address | What it does |
|---|---|
| `sub_823B1CB8` | The engine's present: Synchronize, two GPU timing callbacks, Present |
| `sub_823F3798` | Renderer initialization; sets the present interval from a renderer setting |
| `sub_824D5750` | Engine wrapper of XGSetSurfaceHeader (`r6` = multisample type) |
| `sub_823FBC90`, `sub_823F98D8`, `sub_823F9058` | Create the engine's surfaces (MSAA from the table at `0x8255E254`), and compute the tile count from the AA level (`divwu` at `0x823FBE20`, `0x823F99DC`, `0x823F9228`) |
| `sub_823F3450`, `sub_823F3490` | Engine wrappers: query Issue(BEGIN) and Issue(END) |
| `sub_823F34F0` | Engine wrapper: spins on GetData |
| `sub_8244F4A8` | Sun flare: issues the occlusion query around a quad at the sun |
| `sub_824521F8` | Sun flare: reads last frame's query; visibility = samples / expected |
| `sub_823B1E58` | AA factor (renderer+1420: level 0/1/2 → 1.0/2.0/4.0) |
| `sub_82343A50` | Engine clock (real time, its own 1/60 limiter) |

## The frame (Port Royal dock, Xenos path)

From an F10 frame log of POTCAWERecomp, run 070:

- **Size:** 2399 draws and 40 resolves per frame.
- **Shadows:** two shadow maps, 1024², pitch 1040.
- **The scene:** 1280×720 with 2× MSAA, in **two vertical tiles** (rows 0–416 and 352–720; color at EDRAM
  tile 0, depth at tile 832). Each tile has 962 draws, the same list replayed by the library's predicated
  tiling, and each tile is resolved: color averaged to RGBA16F, depth raw.
- **Per tile:** a full-screen pass, then the transparent objects.
- **Post-processing:** a bloom chain (320 and 160 pitch buffers) and a luminance chain, then the composite,
  the HUD (8888), and the final resolve to the front buffer.
- **The 19 EDRAM clears per frame** are drawn rectangles (`prim=8`, 3 indices, VS `0A6D1DD7`) through
  surfaces with another configuration:
  - the 1280-pitch 2× MSAA scene through 640 pitch at 4× MSAA;
  - the 1040-pitch shadow maps through 1040/520;
  - the bloom buffers at half pitch;
  - the HUD at 640.

  Phase 1 has to establish whether these are XDK Clear calls or engine draws.
- **One occlusion query per frame per tile** (the sun flare). Only the last tile's block is marked.

## Behavior XenosNative has to respect

- **Present interval.** The game sets 2 (30 FPS). `src/framerate.cpp` turns it into 1 for the 60 FPS option,
  and the engine clock keeps the speed right. This keeps working, because Swap still runs through the SDK.
- **GPU waits.** The library's six wait loops are made to sleep by `src/gpu_wait.cpp`. This keeps working,
  because the command processor still advances the read pointer and the fences.
- **Antialiasing option.** `src/msaa.cpp` forces no MSAA (six surface creations) and one tile (three tile
  counts). With XenosNative the scene is drawn once anyway, so the option can become plain "2× MSAA on/off"
  without the tile hooks. To decide in Phase 3.
- **Sun flare.** The expected sample count scales with the AA level (`PotcaweFlareAaLevelHook`). Host
  queries must count samples the way the game expects. Compare with the Xenos path (patch 0020) at the
  same spot.

## Measurements

The baseline and the estimate are in the plan
([Baseline to beat](../xenos_native/docs/plan.md#baseline-to-beat)). Results go here as they come.

Port Royal dock, default camera, window 1280×720, RTX 4070 Ti. GPU % from 20 s of `nvidia-smi`, FPS from
RTSS.

| Date | Build | Configuration | Xenos path | XenosNative |
|---|---|---|---|---|
| 2026-10-13 | Phase 0 (Xenos path only) | 2×2, 2× MSAA, 30 FPS | 42.5 % (repeat 41.1 %) | |
| 2026-10-13 | Phase 0 | 2×2, AA Off, 30 FPS | 27.1 % | |
| 2026-10-13 | Phase 0 | 3×3, 2× MSAA, 30 FPS | 82.7 % | |
| 2026-10-13 | Phase 0 | 3×3, AA Off, 30 FPS | 51.2 % | |
| 2026-10-13 | Phase 0 | 2×2, 2× MSAA, 60 FPS | 82.6 %, 60 FPS | |
| 2026-10-13 | Phase 0 | 2×2, AA Off, 60 FPS | 50.8 %, 60 FPS | |
| 2026-10-13 | Phase 0 | 3×3, 2× MSAA, 60 FPS | 90.5 %, 33 FPS (29.1 ms) | |
| 2026-10-13 | Phase 0 | 3×3, AA Off, 60 FPS | 96.9 %, 60 FPS (17.1 ms) | |

## Open questions (Phase 1)

| Question | Answer | Evidence |
|---|---|---|
| Does the CPU call each scene draw once per frame (tiling replay on the GPU side only)? | | |
| Are the 19 clears XDK Clear calls or engine draws through aliases? | | |
| Does the engine write PM4 directly, or use precompiled command buffers? | | |
| Does any shader use memexport? | | |
| Does the CPU read any resolve result? | | |
| How are the Bink videos drawn? | | |

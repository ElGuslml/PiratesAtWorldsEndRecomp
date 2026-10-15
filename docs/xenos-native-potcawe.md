# XenosNative in POTCAWE

The game side of XenosNative for Pirates of the Caribbean: At World's End: the game's Direct3D layer, its
device layout, how it renders a frame, and how it is measured. For the renderer itself see
[`../xenos_native/README.md`](../xenos_native/README.md). The work plan is
[`../xenos_native/docs/plan.md`](../xenos_native/docs/plan.md).

Status: **Phase 1 done (2026-10-13).** Every entry point XenosNative needs is mapped, and the observer
rebuilds the GPU state of 100 % of the draws at the title screen, the main menu and the Port Royal dock,
with and without 2× MSAA.

**Phase 2 in progress.** With `--xenos_native=true`, XenosNative draws every frame from the language menu
to the Port Royal dock, with placeholder textures ([Phase 2 so far](#phase-2-so-far)).

Sources:
- static analysis of the generated C++ of this build (Phase 1, `local_docs/02-d3d-map.md`);
- the trace and the observer (runs `potcawe_012`–`021`, `local_docs/03-trace.md` and
  `local_docs/04-observer.md`);
- POTCAWERecomp's investigation notes (`local_docs/01`–`06` in that repository) and its hooks
  (`src/gpu_wait.cpp`, `src/framerate.cpp`, `src/msaa.cpp`);
- the RenderDoc analysis of the Port Royal dock.

## Contents

1. [The executable](#the-executable)
2. [The D3D entry points](#the-d3d-entry-points)
3. [The device layout (XDK 2.0.5426)](#the-device-layout-xdk-205426)
4. [Engine functions that matter for rendering](#engine-functions-that-matter-for-rendering)
5. [The frame](#the-frame)
6. [Phase 1 questions, answered](#phase-1-questions-answered)
7. [Behavior XenosNative has to respect](#behavior-xenosnative-has-to-respect)
8. [Phase 2 so far](#phase-2-so-far)
9. [Phase 3 so far](#phase-3-so-far)
10. [Phase 4 so far](#phase-4-so-far)
11. [Measurements](#measurements)

## The executable

| | |
|---|---|
| File | `default.xex` (original name `PiratesXT_XE.exe`), title ID `425607D1`, CRC32 `A31B1069` |
| XDK | 2.0.5426.0 (linked against; minimum 2.0.4552.0) |
| CRT | `0x822A0000`–`0x822A7xxx` (`memcpy` `sub_822A60D8`, `memset` `sub_822A6410`) |
| XGraphics helpers (surface and texture headers, locks) | `0x822AA000`–`0x822AD000` |
| D3D device code | `0x822AD000`–`0x822C7000` |
| D3D draw functions | `0x824ABAD8`–`0x824AC818`, apart from the rest (found only by their `DRAW_INDX` packets) |
| Engine renderer | `0x823B1000`–`0x823FFFFF` and the effects at `0x8244xxxx`–`0x8245xxxx` |
| Videos | Bink (`.bik`) |

## The D3D entry points

The hooks are in [`../src/xenos_native_potcawe.cpp`](../src/xenos_native_potcawe.cpp), one line each.
Status:
- **known**: the packets the function writes, or the device fields it reads and writes, leave no doubt;
- **traced**: also confirmed by the trace (call counts, argument values);
- **likely**: structural evidence only.

### Draws

| Address | Function | Status | Evidence |
|---|---|---|---|
| `sub_824AC3C0` | `DrawIndexedVertices(dev, prim, baseVertex, startIndex, indexCount)` | traced | Flushes every dirty group, writes `VGT_INDX_OFFSET` = baseVertex, then `DRAW_INDX` (`0xC0032201`) with the index buffer `[dev+12300]`. Splits draws over 65535 indices. Sites: `sub_823DF278` (the engine's `DrawIndexedPrimitive`), `sub_824102A0`, `sub_824244E0`, `sub_82453048`. |
| `sub_824ABFD8` | `DrawVertices(dev, prim, startVertex, vertexCount)` | traced | `DRAW_INDX` with 2 dwords (auto index). Site: `sub_823DF2C8` (`DrawPrimitive`). One call per frame at the dock. |
| `sub_824ABF78` | `DrawVerticesUP(dev, prim, vertexCount, data, stride)` | traced | `BeginVertices`, copies the vertices into the command buffer, inlines the end (`[dev+48] = [dev+13300]`). 7 engine sites; 22–26 calls per frame at the dock (post-processing quads, the sun flare). |
| `sub_824ABAD8` | `BeginVertices(dev, prim, vertexCount, stride)` → pointer | traced | Writes fetch slot 31 (vertex fetch 93–95) in the packet, with stream 0 in the command buffer. Only called from `DrawVerticesUP`. |
| — | `EndVertices`, `DrawIndexedVerticesUP` | not present | — |

### Render targets, clears, resolves, tiling

| Address | Function | Status | Evidence |
|---|---|---|---|
| `sub_822B4F60` | `SetRenderTarget(dev, index, surface)` | traced | `+12304 + 4·index`; copies `surface+28` into `RB_COLOR_INFO`. The engine calls it through the alias `sub_822B55A0` (a `b`). |
| `sub_822B52C8` | `SetDepthStencilSurface(dev, surface)` | traced | `+12320`; writes `RB_DEPTH_INFO`, `RB_SURFACE_INFO`, `RB_HIZCONTROL`. |
| `sub_822C6AA0` | `Clear(dev, count, rects, flags, D3DCOLOR, Z=f1, stencil)` | traced | Converts the color and calls ClearF. 2 per frame at the dock. |
| `sub_822C6978` | `ClearF(dev, flags, rect, color, Z=f1, stencil)` | traced | Flags `0x3f` (color + depth + stencil), `0x30` (depth + stencil), `0xf` (color). Implemented as rectangles through an aliased surface (below). |
| `sub_822C2048` | `Resolve(dev, flags, srcRect, destTexture, destPoint, level, slice, clearColor, Z=f1, …)` | traced | Flags `0` = color, `0x10`/`0x14` = depth. 30 per frame at the dock (AA Off), 39 with 2× MSAA. |
| `sub_822BCC58` | `BeginTiling(dev, flags, count, tileRects, clearColor, Z=f1, stencil)` | traced | With 2× MSAA: count 2, tiles (0, 0, 1280, 416) and (0, 352, 1280, 720); 3 per frame. Not called with Antialiasing Off. |
| `sub_822BD1E0` | `EndTiling(dev, resolveFlags, resolveRects, destTexture, clearColor, Z=f1, stencil, params)` | traced | Called with no resolve: the engine resolves each tile itself (below). |
| `sub_822BCAE8` | `SetPredication(dev, mask)` | traced | Masks `0x3` (tile 0) and `0xC` (tile 1) around the per-tile resolves; `0x80000000` inside EndTiling. |
| `sub_822BD698`, `sub_822BD9E0` | `BeginZPass(dev, flags)`, `EndZPass(dev)` | likely | Called conditionally by the engine's target setup; never called in the traced scenes. |
| `sub_822B4E28` | `SetViewport(dev, viewport)` | traced | 41–43 per frame; floats at `+12512`. |
| `sub_822B4550` | `SetScissorRect(dev, rect)` | traced | `+12540`; applied when `+11720` (scissor test enable, `sub_822B4E18`) is set. |

### Shaders, vertex input, textures, constants

| Address | Function | Status | Evidence |
|---|---|---|---|
| `sub_822B7E18` | `SetVertexShader(dev, shader)` | traced | `+12560`. ~315 per frame at the dock. |
| `sub_822B7B60` | `SetPixelShader(dev, shader)` | traced | `+12556`. ~406 per frame. |
| `sub_822B7FE8` | `SetVertexDeclaration(dev, declaration)` | traced | `+11684`, dirty `+16` bit 19. |
| `sub_822B46A0` | `SetStreamSource(dev, stream, buffer, offset, stride)` | traced | Vertex fetch `95 − stream` at `+1784 − 8·stream`; ~1039 per frame. |
| `sub_822B47C0` | `SetIndices(dev, buffer)` | traced | `+12300`. |
| `sub_822AC668` | `SetTexture(dev, sampler, texture)` | traced | Fetch slot `sampler` at `+1024 + 24·sampler`, from `texture+28`. ~4048 per frame. |
| `sub_822B3B70`, `sub_822B3CF8` and neighbours | sampler state setters `(dev, sampler, value)` | likely | Patch fetch dwords 3/4; per-sampler bytes at `+11788`, `+11866`. |
| `sub_822B7848` | `SetVertexShaderConstantF(dev, start, data, count)` | traced | `+1792 + 16·start`, dirty `+0`. ~2371 per frame. |
| `sub_822B7950` | `SetPixelShaderConstantF(dev, start, data, count)` | traced | `+5888 + 16·start`, dirty `+8`. 81 per frame. |
| — | inline constant writes | known | About 44 engine functions write `+1792…`/`+5888…` and set the dirty bits without a call (`sub_823F3590`). |

About 90 render-state setters are mapped by the register they write (`local_docs/02-d3d-map.md`). Most are
called once, from the engine's renderer initialization `sub_823F3798`. XenosNative does not hook them: it
reads the register shadow.

### Resources, synchronization, frame

| Address | Function | Status | Evidence |
|---|---|---|---|
| `sub_822ACEE0`, `sub_822ACF30` | vertex buffer Lock, Unlock | traced | Thunks over the common Lock `sub_822ACA58` and Unlock `sub_822ACC90`. 3–4 per frame, on the render thread. |
| `sub_822ACF40`, `sub_822ACF88` | index buffer Lock, Unlock | traced | 1–3 per frame. |
| `sub_822AA700` | texture Unlock | traced | Never called in the traced scenes. |
| `sub_822AF398` | flush the CPU cache over a range (`dcbf`) | traced | Called by Unlock and by the command buffer code (~430 per frame). |
| `sub_822AEC50` | start a new command buffer segment | traced | When `[+48] > [+56]`; 8–17 per frame, always inside another call. Ends the segment with a fence (`sub_822AE440`). |
| `sub_822AE950` | `NewSegment(dev, dwords)`: the write pointer to a newly allocated segment | hooked (D26) | Sets `+48/+52/+56`. Called by `sub_822AEC50` (through `sub_822AEAC8`), by the reservation `sub_822AEE50` when a reservation does not fit after the kick, and by Present. |
| `sub_822B8120` | `SetShaderGPRAllocation(dev, flags, vertexGprs, pixelGprs)` | traced | 7–8 per frame (48/80 and 16/112 GPRs). Runs three prebuilt command buffers (`INDIRECT_BUFFER`) of 8 one-point draws each, at `0x1FCA1000` (physical). |
| `sub_822BA590`, `sub_822BA818` | Query Issue, GetData | Issue hooked (D32), GetData traced | 4 Issue and 1 GetData per frame (the sun flare): two queries, each BEGIN then END. |
| `sub_822B58B0` | `Swap(dev, frontBuffer, params)` | traced | Once per frame, from the engine's Synchronize (`sub_822B5D50`, an alias with no front buffer). |
| `sub_822B5D58` | `Present(dev, params)` | traced | Once per frame; ends the frame. Does not call Swap. |
| `sub_822AF860` | `Direct3D_CreateDevice` | known | Calls the device init `sub_822B6A60` (ring init with `ME_INIT`, prebuilt buffers by `sub_822B7700`). |
| — | `InsertFence`, `BlockOnFence` | not used | No such function is called. The game synchronizes through Lock (resource fences) and Swap. |

Internal functions (do not hook): the dirty-state flush is inline in each draw. It is done by:
- `sub_822B02F0`: float constants;
- `sub_822AFF50`: one register group;
- `sub_822B0190`: fetch constants;
- `sub_822B0010`: misc;
- `sub_822AFBB8`: state that depends on the tile;
- `sub_822B1090`: the program flush, which loads the shaders and their literal constants (`sub_822B0EA0`
  writes `LOAD_ALU_CONSTANT`).

## The device layout (XDK 2.0.5426)

As passed to the library (`potcawe::XenosNativeProfile()`). The device pointer is constant for a run
(`0x40002600` in every traced run).

| Offset | Field | Evidence |
|---|---|---|
| `+0`…`+40` | six u64 dirty masks: VS constants, PS constants, register groups and program, fetch slots (low 32 bits: slot s = bit 31 − s) and group `0x2280`, misc and bool/loop, tiling-dependent state | draws; fetch flush `sub_822B0190` (`cntlzd`) |
| `+48` / `+52` / `+56` | command buffer write pointer (**address of the last dword written**: the library writes with `stwu`) / second limit / kick threshold | every packet writer |
| `+1024` | fetch constants, 32 × 24 bytes | SetTexture |
| `+1792` | VS float constants, 256 vec4 (flushed to `0x4000`) | SetVertexShaderConstantF |
| `+5888` | PS float constants, 256 vec4 (flushed to `0x4400`) | SetPixelShaderConstantF |
| `+9984` | bool (8 dwords) and loop (32 dwords) constants | flush |
| `+10240` / `+10316` / `+10400` / `+10420` / `+10468` / `+10552` / `+10704` | register shadow groups `0x2000`+16, `0x2100`+21, `0x2180`+5, `0x2200`+12, `0x2280`+21, `0x2300`+38, `0x2380`+8 | flush |
| `+10768` | pointer to the write-back block: `[+0]` last fence the GPU reached, `[+4]` its ring position | `sub_822AE440` |
| `+10772` | pointer to the swap/vblank block | POTCAWERecomp |
| `+10780` | **fence counter** (steps by 2 per segment; stamped into resources at `+8`) | `sub_822AE440`, SetIndices |
| `+10812`…`+10815` | flag bytes (`+10812` bit 4: tiling) | many |
| `+11684` | vertex declaration | SetVertexDeclaration |
| `+11716` | D3DRS_PRESENTIMMEDIATETHRESHOLD | POTCAWERecomp |
| `+11720` | D3DRS_SCISSORTESTENABLE | `sub_822B4E18` |
| `+11892` | sRGB write state | `sub_822B34E0` |
| `+12300` | index buffer | SetIndices |
| `+12304`…`+12316`, `+12320` | RT0–RT3, depth-stencil | SetRenderTarget, SetDepthStencilSurface |
| `+12324 + 4·s` | vertex buffer of stream s | SetStreamSource |
| `+12408 + 4·i` | texture of sampler i | SetTexture |
| `+12512`, `+12540` | viewport (6 floats), scissor rect | SetScissorRect |
| `+12556`, `+12560` | pixel shader, vertex shader | SetPixelShader, SetVertexShader |
| `+12572` | predication mask | SetPredication |
| `+12592`…`+12608` | targets saved by BeginTiling | BeginTiling |
| `+12612`, `+12616` | tile count, tile rects (x1, y1, x2, y2) | BeginTiling |
| `+13300` | command buffer pointer saved by BeginVertices | DrawVerticesUP |
| `+13444` | D3DRS_PRESENTINTERVAL | POTCAWERecomp |

**Correction:** the notes inherited from POTCAWERecomp called `+10780` the ring write pointer. It is the
fence counter; the write pointer is `+48`.

Shader objects (as in Dante's Inferno):
- **Pixel shader:** program block at `PS + [PS+64]`. Microcode at `[PS+24] + [block+40]`, size
  `[block+44]` bytes. `SQ_INTERPOLATOR_CNTL` is `[block+64]`. Literal constants are loaded from `PS+40`.
- **Vertex shader:** microcode base `[VS+32]`, the active variant at `VS + [VS+896] + 872`. Literal
  constants are loaded from `VS+872`.

## Engine functions that matter for rendering

| Address | What it does |
|---|---|
| `sub_823B1CB8` | The engine's present: `SetShaderGPRAllocation(0, 0, 0)`, a GPU callback, Synchronize, timing, Present |
| `sub_823DF3A8` | Draws every scene mesh, through `sub_823DF278` (`DrawIndexedPrimitive`) and `sub_823DF2C8` (`DrawPrimitive`), which turn primitive counts into index counts with the table at `0x820349A8` |
| `sub_823FA010` | Binds the targets (SetRenderTarget, SetDepthStencilSurface), then BeginTiling and BeginZPass when tiling |
| `sub_823FA1A8`, `sub_823FB150` | End of a pass: SetPredication per tile, the per-tile resolves, EndTiling |
| `sub_823F3798` | Renderer initialization: applies a full render state block, sets the present interval |
| `sub_823F3590` and ~44 others | Write shader constants straight into the device |
| `sub_823F3450`, `sub_823F3490`, `sub_823F34F0` | Query Issue(BEGIN), Issue(END), and a spin on GetData |
| `sub_8244F4A8` | Sun flare: the occlusion query around a quad at the sun (3 DrawVerticesUP) |
| `sub_824521F8` | Sun flare: reads last frame's query |
| `sub_824D5750` | XGSetSurfaceHeader wrapper (`r6` = multisample type) |
| `sub_823FBC90`, `sub_823F98D8`, `sub_823F9058` | Create the engine's surfaces and compute the tile count from the AA level |
| `sub_823B1E58` | AA factor (renderer+1420: level 0/1/2 → 1.0/2.0/4.0) |
| `sub_82343A50` | Engine clock (real time, its own 1/60 limiter) |
| `sub_8242D218` | Per mesh, every frame, under the scene traversal (`sub_8233AC68` ↔ `sub_82337DB8`, recursive, then `sub_82351650`, `sub_82430360`, `sub_8242D990`): updates the mesh's flags and takes and releases a reference (atomic, `+4`) on its material objects. Those writes land in the same 256 KB blocks of physical memory as about 30 textures (see Phase 2 so far) |

The engine's renderer object keeps the D3D device at `+116`.

## The frame

### Port Royal dock, 2× MSAA (log `potcawe_013`, frame 2445)

11 503 D3D calls on one thread. The draws:
- 1100 `DrawIndexedVertices`;
- 1 `DrawVertices`;
- 26 `DrawVerticesUP`.

The passes, in order:

1. **A small pass at pitch 160** (40 draws), resolved.
2. **Two shadow maps:** depth only, pitch 1040, 22 and 8 draws. Each is cleared with ClearF(`0x30`) and
   resolved as depth into slices 0 and 1 of one texture.
3. **The scene: a tiled pass.**
   - BeginTiling(2 tiles), then ClearF(`0x3f`) of color and depth, through the alias.
   - 992 draws, plus the sun flare query.
   - Then, for each tile: SetPredication(tile mask), then a Resolve of color and of depth with the
     tile's rectangle, into the same two textures.
   - Then EndTiling, with no resolve of its own.
4. **Two more short tiled passes** of 1–23 draws each, with the same per-tile resolves.
5. **Post-processing at pitch 320, 160 and 80:**
   - about 20 one-quad passes (`DrawVerticesUP`), each followed by its resolve;
   - the bloom and luminance chains, cleared with Clear(`0xf`).
6. **The composite and HUD** at 1280×720, and the final resolve into the front buffer. The two front
   buffers, `0x40000770` and `0x40007cc0`, alternate.

Per frame there are:
- 39 resolves;
- 11 ClearF (2 nested in Clear);
- 3 BeginTiling and 3 EndTiling;
- 4 query Issue;
- 7 SetShaderGPRAllocation;
- 17 segment starts.

### Port Royal dock, Antialiasing Off (logs `potcawe_012`, `020`)

The same frame without tiling:
- no BeginTiling or EndTiling;
- the scene is one 987–992-draw pass with a full-screen resolve;
- 30 resolves and 7 ClearF per frame.

### Menus and title (logs `potcawe_012`, `020`, `021`)

| Scene | Calls | Draws (indexed + UP) |
|---|---|---|
| Language menu | 153 | 16 + 1 |
| Title screen (with the Bink background) | 205–278 | 12–13 + 3–5 |
| Main menu | 230–242 | 18 + 3 |

Each one ends with the composite and a resolve into the front buffer.

### What the GPU executes besides the game's draws

The command processor runs more draw packets than the game issues. In the observer's frames, every one is
the library's own:

| Source | Draws | What they are |
|---|---|---|
| ClearF / Clear | 2–22 per frame | `RECTLIST` of 3 vertices through an aliased surface: the 1280-pitch 2× MSAA scene as pitch 640 at 4× MSAA (`RB_SURFACE_INFO` `0xa020280`), the 1040-pitch shadow maps as pitch 520 (`0x8020208`). Mode 4 (color) or 5 (depth). |
| Resolve | 6–40 per frame | `RECTLIST` of 3 vertices in copy mode (`RB_MODECONTROL` 6). |
| SetShaderGPRAllocation | 144–192 per frame | `POINTLIST` of 1 vertex from prebuilt buffers at `0x1FCA1000`, with their own shaders. They draw nothing visible. |

## Phase 1 questions, answered

| Question | Answer | Evidence |
|---|---|---|
| Does the CPU call each scene draw once per frame (tiling replay on the GPU side only)? | **Yes.** With 2× MSAA, the 1021 draws inside tiled passes run twice on the GPU from one CPU call. What differs between the two runs: `PA_SC_WINDOW_OFFSET` and `PA_SC_WINDOW_SCISSOR_TL/BR` (the tile's position, from bin-predicated packets), the occlusion query address, and nothing else (no constant, no render state). The engine resolves each tile itself, under SetPredication, into the same texture; EndTiling resolves nothing. | Observer (log `potcawe_019`): executions 2× for 1021 calls, 1× for 106; tile differences limited to those registers. Trace (log `potcawe_013`). |
| Are the 19 clears XDK Clear calls or engine draws through aliases? | **Library ClearF calls.** ClearF draws its rectangle through a surface it configures itself in the packet (half the pitch at 4× MSAA for 2× MSAA targets). 9–11 ClearF per frame, some inside tiled passes (run per tile). XenosNative can clear the bound host targets instead. | `sub_822C5830` writes `RB_SURFACE_INFO` in the packet; observer attributes every alias draw to ClearF/Clear. |
| Does the engine write PM4 directly, run precompiled command buffers, or patch the command buffer? | **No.** Every draw packet in the verified frames was written by a hooked D3D call. The only other buffers are the library's own (SetShaderGPRAllocation). The PM4 scan finds no packet builder outside the D3D library. | Observer: no draw packet "written outside any hooked call"; `tools/pm4_consts.py`. |
| Does any shader use memexport? | **No**, in the verified scenes (title, menus, dock). | Observer (the SDK's shader analysis). |
| Does the CPU read any resolve result? | **No sign of it.** No texture is locked in any traced frame, and no engine code locks a resolve target. A read through a raw pointer is not excluded: to watch in Phase 3, where a missing readback would show as stale data. | Trace: TextureUnlock 0 per frame. |
| How are the Bink videos drawn? | **Through the same D3D calls.** The title screen's background is a Bink video: three `k_8` textures (Y 1280×720, Cb and Cr 640×360) and one `DrawIndexedVertices` quad with a YUV pixel shader. The planes are written by the CPU **without Lock/Unlock**, so XenosNative has to notice CPU writes to them (as the SDK does with page watches). | Trace (log `potcawe_021`, draw #48); observer 18/18. |

## Behavior XenosNative has to respect

- **Present interval.** The game sets 2 (30 FPS). `src/framerate.cpp` turns it into 1 for the 60 FPS option,
  and the engine clock keeps the speed right. This keeps working, because Swap still runs through the SDK.
- **GPU waits.** The library's six wait loops are made to sleep by `src/gpu_wait.cpp`. This keeps working,
  because the command processor still advances the read pointer and the fences.
- **Antialiasing option.** `src/msaa.cpp` forces no MSAA (six surface creations) and one tile (three tile
  counts). With XenosNative the scene is drawn once anyway, so the option can become plain "2× MSAA on/off"
  without the tile hooks. To decide in Phase 3. Until then the hooks always apply with XenosNative
  (`xenos_native::Renders()`, D25): with 2× MSAA the scene surface is 1280×416 (one tile), and XenosNative
  drew only those top 416 rows of the image (run 46).
- **Sun flare.** The expected sample count scales with the AA level (`PotcaweFlareAaLevelHook`). Host
  queries must count samples the way the game expects. Compare with the Xenos path (patch 0020) at the
  same spot. With tiling, each tile has its own query block (the address is the only state that differs
  per tile). Done in Phase 3 (D32): see [Phase 3 so far](#phase-3-so-far).
- **Per-tile resolves.** The engine resolves each tile's rectangle into the same texture under
  SetPredication. With one host target, these become one resolve of the union (the overlap rows are
  resolved twice with the same content).
- **Graphics API.** With XenosNative, `src/options.cpp` sets `gpu_backend = vulkan` for the SDK's
  sync-only backend (D27); otherwise an overlay such as RTSS may report the idle D3D12 device.
- **Video planes.** Bink writes its texture planes without Lock/Unlock. XenosNative's texture cache sees
  those writes through the guest memory mirror's page watches, and reloads a plane when its data changed
  (D23). The title screen and main menu backgrounds play.

## Phase 2 so far

Runs `potcawe_026`–`074` (`local_docs/05-phase2.md`), 2026-10-13 and 14, window 1280×720, default
settings: **Antialiasing Off** (`potcawe_no_msaa = true` in `potcawe_options.toml`; every render target is
1×) and 60 FPS. (Corrected on 2026-10-14: earlier versions of this section and of the plan said 2× MSAA.)

| Scene | Draw calls per frame | Drawn | Render targets / resolve targets | Shaders / pipelines | Image |
|---|---|---|---|---|---|
| Menus (language, title, main menu) | 2–28 | all | 8 / 20 | 11 / 8 | Since step 7, as on the Xenos path, Bink backgrounds included. |
| Port Royal dock | 1,120–1,144 | all | 12 / 33 | 97 / ~180 (growing slowly) | Since step 7, as on the Xenos path at the same spot: textures, HUD, character shadows. |

What the game does that the draw path met:
- **Every draw packet is bin-predicated**, also outside tiled passes. The draw's primitive type, count and
  index buffer are in that packet (`VGT_DRAW_INITIATOR`, `VGT_DMA_BASE`, `VGT_DMA_SIZE`).
- **Index buffers** at the menu are 16-bit with the 8-in-16 swap (`VGT_DMA_SIZE` `0x4000xxxx`); the panels are
  auto-indexed triangle strips.
- **Inline vertex shaders seemed overwritten, and draws kept the wrong programs.** Vertex shaders are
  loaded inline (`IM_LOAD_IMMEDIATE`, 105–192 dwords) by `DrawIndexedVertices`. Sometimes the copy held a
  coherency packet sequence (`COHER_*_HOST`, `WAIT_REG_MEM`, as `sub_822B0010` writes it) in the middle of
  the microcode, which made the SDK's analysis crash the game (D20, D21). The cause (D26) was a segment
  change the tracker did not follow: when a reservation does not fit even after the kick, the library
  starts another segment directly (`sub_822AE950`). The tracker then read stale memory as packets, or lost
  the call's packets. The lost ones were worse: one or two calls per frame, and when a draw's program flush
  was lost, that draw and the next ones (up to 24) kept the previous programs. Those were the stretched
  triangles Gustavo saw at the dock for a few seconds at a time. With `NewSegment` hooked, every draw
  matches the command processor, programs included.
- **Texture addresses.** The resolve destinations' headers hold CPU addresses (`0xA84D2000`); the sampling
  draws' fetch constants hold GPU addresses (`0x084D2000`). Once both were keyed by the GPU address, the
  composite found the scene: 156 texture bindings per frame at the dock are resolve results, the other
  ~9,500 are placeholders until step 7.
- **Uploads.** About 1.5 MB of vertex data in about 170 copies per frame at the dock, most of it data the
  game writes every frame.
- **GPU time.** 10.7 ms per frame at the dock before step 7, 11.7–12.0 ms with the textures (DiligentCore
  duration queries), against 8.5–9 ms for the Xenos path at 2×2 with AA Off (four times the pixels).
  Renderer CPU time 4.7 ms per frame before step 7, 5.4 ms after. Phase 3 has to close that gap.
- **Lost device on the first dock frames.** Some buffer read of the translated shaders goes out of range
  there; without Vulkan's `robustBufferAccess` the device was lost, which DiligentCore does not report in
  release builds. It showed up as a dynamic heap that filled up every 1.4 s and a GPU at 8.6 %. Fixed by
  enabling the feature (D22).

Step 7, textures (runs `potcawe_056`–`074`):
- **About 4,000 textures** are loaded by the time the dock is on screen (1.8 GB of host data, much of it
  DXT1 decompressed to RGBA8); at the dock about 9,500 texture bindings per frame are guest textures and
  about 160 are resolve results. No format met so far lacks a host format.
- **The engine writes next to its textures every frame.** `sub_8242D218` (see the engine functions) writes
  reference counts in pages of the 256 KB blocks that hold about 30 of the dock's textures, at the start
  of each frame, on the render thread. `SharedMemory` invalidates whole blocks, so those textures became
  outdated every frame, 16 MB of host data reloaded with unchanged guest data. The texture cache now
  hashes the data first (D23): 31 loads skipped per frame, a few real loads per minute.
- **The back buffer surface changes format.** Surface `0x40243200` (and the scene's `0x40243E70`) reads
  `2_10_10_10_FLOAT_AS_16_16_16_16` at its draws and `2_10_10_10_FLOAT` at its resolves. With host targets
  keyed by the surface's format, the HUD composite was drawn into one target and the front buffer resolved
  from the other, so the HUD never showed. Keyed by the storage format now.
- **The HUD** is drawn into its own 8_8_8_8 surface at EDRAM 0 (cleared to transparent black; the 3D
  elements first, with reversed depth, then a depth resolve, then the 2D elements), resolved to
  `0x0886B000`, and composited over the scene in the back buffer (2 DrawVerticesUP).
- **Character shadows** come from two cascades drawn into a 1024×1024 float24 depth surface (pitch 1040)
  and resolved into slices 0 and 1 of one stacked `k_24_8_FLOAT` texture (`0x08FAA000`). The scene's pixel
  shaders (for example `6B954BAB187F64D2`) take four point samples (`tfetch3D`, the slice from the cascade
  selection) and compare them with the light-space depth they compute. Both the slices (once both
  cascades landed in one host slice) and the depth range (D24) had to be right for the shadows to show.
- **The front buffer** is resolved with the red/blue swap, and its texture's swizzle swaps it back; the
  presenter applies that swizzle now.

Gustavo's play at the dock (2026-10-14, after D26):
- no stretched triangles and no white patch;
- the sun glare is too strong and drawn over the geometry when turning right: the flare's occlusion
  queries get fake counts on this path (Phase 3);
- the GPU load depends on the view: ~80 % at the start view, 20–25 % facing the sea (RTSS), against ~30 %
  for the Xenos path at the start view with the same settings (Phase 3, performance).

## Phase 3 so far

Notes: `local_docs/06-phase3-performance.md`. Window 1280×720, Antialiasing Off, 60 FPS.

**GPU time (D30).** Every draw ran in its own render pass. Binding the render targets only when they change
took the dock from 12.1 to 3.3 ms per frame facing the town, and from 2.8 to 1.8 ms facing the sea.

**The sun flare's occlusion queries (D32).** Runs `potcawe_101`–`108`:
- per frame, two queries, each Issue(BEGIN), the flare's draws, Issue(END). With one tile their blocks are
  `0x06F2D000` and `0x06F2D040`, the same every frame;
- the counts: up to 91 samples per query on the first frames at the dock, 0 with the sun hidden or off
  screen, up to 104 facing it;
- 1,200 queries answered per 600 frames. Only the first 4 at the dock come before any result, and get 0;
- in three views toward the sun, each taken on both paths with the same keys, XenosNative's glare
  matches the Xenos path's.

**Textures (D33).** Runs `potcawe_109`–`113`:
- the dock keeps 412–460 guest textures on the host. With DXT1 decompressed they took 276–318 MB; as BC1,
  128–139 MB. The video memory in use went from 3,968 to 3,763 MB (RTSS);
- the "1.8 GB of host data" of Phase 2's step 7 is the data the loads wrote from the start to the dock,
  most of it the menus' video planes, which every frame of video loads again. It is not what stays on the
  host;
- the alpha-tested foliage looks as before, with no black borders: BC1 with 1-bit alpha.

**Shader translation (D34).** The dock's load analyzes 83 shaders and translates 93 shader variants.
On worker threads, the render thread's longest frame in the load went from ~80 ms to 52–54 ms. About 1,400
draws are skipped over the load's first frames while their shaders are translated, and the start view is
complete 25 s later.

**Guest memory uploads (D35).** Each frame at the dock asks again for about 150 ranges (~3 MB) of index,
vertex and texture data, because the engine's per-mesh reference counting (`sub_8242D218`) writes next to
them and the mirror's invalidation covers more than the write. Only 6–7 of them hold new data: vertex data
written just before draws 41, 607 and 1062–1122, and one index range. With unchanged pages skipped the dock
makes 7–9 copies per frame, and its GPU time is 1.55–1.66 ms facing the town (3.3 ms before).

**Render resolution (D37, D38).** The profile gives 1280×720 as the game's output size. The scale
follows the screen:
- what scales: every surface the game renders, including the 1024² shadow maps (2048² at 2×) and the
  luminance chain's 1×1, 4×4, 16×16 and 64×64 targets (2×2 to 128×128 at 2×);
- parity: at exactly 2× (a 2560×1440 monitor) the image matches the Xenos path at 2×2 more closely than
  at 1×;
- the sun's occlusion counts stay at the game's scale (92–98 samples per query at 1.067×, 1.25× and 2×);
- the launcher: with XenosNative, its render quality row is disabled and shows the resolution XenosNative
  will use, computed from the monitor or the window size the way the renderer computes it
  (`NativeRenderSize` in `launcher/potcawe_launcher.cpp`). Its XenosNative hint says it renders at the
  screen's resolution, in the five languages. The scale choices stay for the Xenos path.
  - **Since D54** the row is "Internal resolution", and with XenosNative it offers automatic or a lower
    height. A new "Image format" row chooses filling a wide screen or 16:9 with bars.

**The display gamma (found while checking D33, fixed by D36).** XenosNative's image was lighter in the
darks than the Xenos path's, and Gustavo found it flat while playing. The SDK's presenters apply the
guest's gamma ramp (`apply_gamma_table` / `apply_gamma_pwl`); XenosNative presented the front buffer as it
was. The tone curve from XenosNative to the Xenos path, green channel, start view (average Xenos level per
XenosNative level):

| XenosNative | 24 | 48 | 64 | 96 | 128 | 160 | 192 | 248 |
|---|---|---|---|---|---|---|---|---|
| Xenos path, before D36 | 10 | 32 | 49 | 82 | 116 | 150 | 186 | 247 |
| Xenos path, with D36 | 23.8 | 47.1 | 64.3 | 96.2 | 127.1 | 159.4 | 191.8 | 247.2 |

POTCAWE presents a `2_10_10_10_AS_16_16_16_16` front buffer (format 54, 1280×720) and writes a piecewise
linear ramp early on (384 DC_LUT writes). Red at 32, 64, 128 and 192 of 255 becomes 67, 196, 467 and 748 of
1023, where a straight line gives 129, 259, 518 and 776. The 256-entry table stays linear. The white-out over the geometry Gustavo saw after D26 is gone.

## Phase 4 so far

**XenosNative is POTCAWE's default (D56):**
- `xenos_native = true` is among the defaults in `src/startup.cpp`, and the launcher selects XenosNative when
  no file names a renderer;
- `xenos_native = false`, or the launcher's Renderer row, gives the Xenos path;
- the Xenos path also draws if XenosNative cannot start.

**The end of the frame** (trace of frame 4500 at the dock, run 156, with the callers):
1. **The post-processing** draws the scene into the composite surface (`0x40243200`, 2_10_10_10_FLOAT,
   pitch 1280, EDRAM 0), and the engine copies it to `0x0886B000`, 8_8_8_8 (Resolve from `0x8245E180`).
2. **A temporal blend:** the last frame's finished scene (`0x09B2E000`) is drawn into the surface, then
   blended with the copy and the depth texture `0x084D2000`. That looks like motion blur.
3. **The finished scene** is copied to `0x09B2E000` (Resolve from `0x824583EC`). It is also the next
   frame's "last frame".
4. **The HUD** is drawn into an 8_8_8_8 surface at the same EDRAM (`0x40268410`): a clear to transparent black,
   5 draws of 3D HUD elements, a depth resolve to `0x084D2000`, 10 draws of 2D elements, then a copy to
   `0x0886B000`.
5. **The composite:** two full-screen `DrawVerticesUP` into `0x40243200`, the finished scene
   (`0x09B2E000`) and the HUD (`0x0886B000`) over it. Then the resolve to the front buffer, format 54.

**Antialiasing (D41).** The profile names step 3's resolve (`scene_resolve_caller = 0x824583EC`).
XenosNative antialiases that copy in place with SMAA or FXAA, so step 5 composites a smooth scene under a
sharp HUD:
- once per frame at the dock;
- in the menus too, where their text stays sharp;
- SMAA costs about 0.05 ms at 1× and 0.15 ms at 2×.

**Pipeline storage (D42).** POTCAWE's runtime keeps it in `userdata/cache/xenos_native/` next to the
executable:
- `425607D1.pipelines`: 97 shaders, 108 translations and 128 pipelines after the menus and the dock,
  77 KB;
- `425607D1.vkcache`: 5.3 MB then, 25 MB after testing many render scales (each adds its own binaries).
  Saving it took 30–80 ms; it is saved on a thread of its own since D45.

From the second run on, the dock loads with no draw waiting for a shader or a pipeline.

**Aspect ratios (D43).** What `src/aspect.cpp` relies on:
- **The camera.** `sub_823288C8` builds the engine camera's projection and culling frustum (three callers).
  - The perspective branch takes the tangents of the half angles: `+12` is the width and `+8` the height,
    in degrees. It passes them to `sub_82328428` (the frustum, with `+16` and `+20`, the depth range),
    then stores P00 = 1 / tan at `+432` and P11 at `+452`. The projection is copied to `+496`.
  - The menus use a 45° vertical angle (tan 0.41421), the dock 55° (0.52057), both 16:9 (tan ratio 1.7778).
  - The parallel branch builds P00 = 2 / `+28` and P11 = 2 / `+32`. The only parallel cameras seen are
    1000×1000.
  - The other camera class, with the "Projection Angle Width/Height" properties (`sub_822FC610`,
    `sub_82303D60`), does not run in the menus or at the dock.
- **The views.** `sub_823B6290` (virtual) draws one view per call, in stages; its return addresses name
  them:
  - `823B6334`: shadow maps, which upload their own matrices from `823B4BC4`;
  - `823B6650`, `823B67AC`, `823B67CC`: opaque 3D;
  - `823B69A4`, `823B6A08`: effects, the sun's haze, the flare query;
  - `823B6AD8`: 2D draws (also the menu's Bink video);
  - `823B6B40`: the post-processing (`sub_823D2160`), which in the last view composites the scene and the
    HUD over the screen;
  - `823B6C48`: the last resolve.

  At the dock a frame has: the water reflection (`823EDF5C`), the 3D view, then the HUD view. In the menus:
  the video's view, then the menu's.
- **Telling 2D views apart.** `sub_823B3C38` sets a view's constants and passes the view's matrix to
  `c0–c3` (the call at `0x823B3E48`, `r5`) and to `c160` of the pixel shader.
  - The 2D views (the HUD, the menus, the video behind them) have a square parallel projection:
    `0.002 0 0 0 / 0 0.002 0 0 / 0 0 z 0 / 0 0 t 1` (the 1000×1000 camera).
  - The 3D views have a perspective column (`m[3] = 1`, `m[15]` = the distance).
- **The text measures through the 2D camera.** The game's text layout converts glyph widths through that
  camera. With its P00 narrowed (run 177) the glyphs kept their stretched width, and "Reiniciar desde punto
  de control" lost its last word. Hence the renderer's box and an untouched camera.
- **The haze's rectangle.** `sub_823F8750` sets the scissor from a draw's screen rectangle, at `+784` of
  its render state block: `+4` left, `+8` top, `+12` width, `+16` height (0 to 1 of the viewport), `+20`
  enabled.
  - At the dock it is used by the sun's haze (a mesh of 900 indices), and in the HUD view (run 156: a
    HUD mesh drawn a second time with it, perhaps a gauge).
  - The town view gives `0 0 1106 720` at 16:9 and `0 0 967 720` at 21:9. The rectangle is computed with
    the camera, but at 21:9 it falls short of the mesh: a vertical seam in the sky toward the sun (RenderDoc,
    `potcawe_frame6849.rdc`, event 1433: a scissor 2340 pixels wide).
  - At 16:9, without the rectangle, the image does not change (runs 182, 183).
- **Anisotropic filtering (D44).** The game's samplers in the menus and at the dock come to 11 distinct
  states.
  - One asks for 4x itself: a texture with 11 mip levels. It only appeared in the run that turns toward
    the sea.
  - The dock's textures (5 and 8 mip levels) ask for none: trilinear.
  - The rest have no mipmaps (render targets, 2D).

  The SDK's `anisotropic_override` (16x by default here, `src/startup.cpp`) applies to the trilinear ones
  on both paths. At 16x the GPU time is unchanged: 3.46 ms at 2×, 6.22 ms at 3×.
- **DLAA (D47).** What `src/views.cpp` and the renderer rely on:
  - **The main view's targets.** The main 3D view draws into surface `0x40243E70` with depth `0x40243EB0`,
    1280×720. Its reflection draws into a 128×128 target, and its shadow maps into 1024² ones.
  - **The main view's passes:** a clear (depth to 0), 992 opaque draws, the sun's occlusion quad, a copy of
    the color (Resolve flags 0x10 from `0x823FB79C`) and of the depth, 24 transparent draws (the haze, the
    water), the sun's quad again. The post-processing then starts at the hook point `0x823B6B20`.
  - **Reversed depth.** ClearF to z = 0, and the transparent draws test `GreaterEqual`; the viewport's z
    goes from 1 (near) to 0 (far).
  - **The camera moves a little every frame,** even standing still. The motion vectors average 0.21 px at
    the dock's idle view, and DLSS needs them.
- **The frame's views and surfaces (D49, probe, run 248).** At the dock, per frame:
  - **The water reflection view:** `BeginView` with a matrix, a clear, 40 draws into 128×128, a resolve to
    `0x0BEA5000`. No `EndViewGeometry`.
  - **The main view:** `BeginView`, then the two shadow maps (1024², resolved into slices 0 and 1 of
    `0x08FAA000`). Then, inside the view, the scene's clear (`0x3f`) of `0x40243E70` and `0x40243EB0`, and
    994 draws, 37 of them sampling resolved textures.
  - **The copies inside the view:** the color to `0x07DA1000` (from `0x823FB79C`) and the depth to
    `0x084D2000` (from `0x823FB7E0`). Then the sun's quad, the depth copy again, and 24 draws (the haze,
    the water), 12 of them sampling resolved textures.
  - **After `EndViewGeometry`:** the post-processing resolves the scene's depth (from `0x82457ED8`) and color
    (to `0x0886B000`). Then the bloom and luminance chain, from 320×180 down to 1×1, the composite, and the
    copy of the finished scene to `0x09B2E000`.
  - **The HUD view:** `BeginView` without a matrix, a clear `0x3f` (its own depth), 14 draws, the composite
    and the resolve to the front buffer.
  - **Parameter generation (VPOS):** no draw uses it.
- **The journal (Back, D50; trace of frame 6323, run 272).** Per frame while it is open:
  - **Each page is composed:**
    - the page's parchment, map and text are drawn into the scene's surface (`0x40243E70`, cleared to
      transparent black) and resolved to `0x0886B000`;
    - a quad then copies that into a 1024×512 surface at EDRAM 0 (pitch 1040, `8_8_8_8`, not cleared);
    - the result is resolved to the page texture (`0x0CC36000`, `0x0CE37000`).
  - **Then the usual frame:** the reflection, the shadows, the scene, the post-processing and the HUD. The
    scene copy `0x09B2E000` is resolved twice from `0x824583EC`, once without and once with the HUD
    composited, so SMAA (D41) runs twice per frame there.
  - **The journal view:** the scene's surface is cleared to alpha 0.627, then 36 draws, a depth copy,
    10 draws, and a resolve to `0x0886B000` (`k_16_16_16_16_EXPAND`). The pages leave alpha above 1 there.
  - **The final composite** draws the scene and HUD copy (`0x09B2E000`) and the journal over it, by its
    alpha, into `0x40243200`, then resolves to the front buffer.
- **A cube map rendered every frame (D51).** In an area of level 3 (Gustavo's session, run 278,
  01:54–02:00, after a loading screen):
  - the game draws a 256×256 target (EDRAM base 0, pitch 320, `k_32_FLOAT`);
  - it resolves the target into the six faces of a cube texture, six resolves per frame;
  - it is likely a point light's shadow map.
- **Object motion (D48).** The main view's vertex shader constants, at the dock:
  - **Shared by the whole view:** c0–c3 (the view-projection matrix, from `sub_823B3C38`), c8, c9 and c27.
    c0–c3 and c9 change whenever the camera moves.
  - **Characters are skinned on the GPU.** Their vertex shaders (`64DBDAAEED0A33A0`, the most common one,
    and `9227FA5CA9D8D397`, `C454E3FEAE46D6E9`) use all 256 registers: the bones in c31–c219, the world
    matrix in c4–c7. Their vertex buffers do not change.
  - **Animated props** use 12 registers; c4–c7 (the world matrix) change from frame to frame.
  - **Counts:** about 960 draws of the main view write depth. About 150 of them move: Jack, the NPCs and
    the props. That holds with the camera still or moving.
- **Device memory at the dock (D55).** In a 1600×900 window, after 8 minutes:
  - 851 MB in use in 880 MB of DiligentCore's pages;
  - guest textures 128–142 MB, render targets 41 MB (12), resolved textures 64 MB (20);
  - about 604 MB fixed, 512 MB of it the guest memory mirror.

  At 3840×2160, in Gustavo's long session (run 278), the guest textures stayed at the SDK cache's soft
  limit (about 383 MB) from level 1 on.
- **A script error closed the game (run 288), not the renderer.**
  - The log showed faults on three threads at once. They came from `exit()` running the game's static
    destructors.
  - The exit came from Lua's `luaD_throw`: the recompiled `longjmp` returned instead of jumping.
  - Fixed on the game side and merged from POTCAWERecomp: `docs/script-errors.md`.
  - **For other games:** a crash with faults on several threads at the same moment, inside destructors,
    may be an `exit()`. Look for the C runtime's exit on the main thread's stack.
- **The menu videos have a green border** one pixel wide (row 1, column 1 and the last column), at full
  width as in the box. It is the Bink planes' edge, older than D43.

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
| 2026-10-14 | Phase 2 close (`cc6003d`) | 1×, AA Off, 60 FPS | ~30 % (RTSS) | ~80 % (RTSS); 12.1 ms GPU (duration queries), 4 ms facing the sea |
| 2026-10-14 | Phase 3, D30 | 1×, AA Off, 60 FPS | | 27 % (RTSS); 3.3 ms GPU, 1.8 ms facing the sea |
| 2026-10-14 | Phase 3, D35 | 1×, AA Off, 60 FPS | | 14–19 % (RTSS); 1.55–1.66 ms GPU facing the town, 1.0–1.7 ms walking |
| 2026-10-14 | Phase 3, D38 | 2× (2560×1440 fullscreen), AA Off, 60 FPS | (2×2) | 3.32–3.84 ms GPU facing the town; 1.25× 1.95 ms, 1.5× 2.35–2.51 ms |
| 2026-10-14 | Phase 3 A/B (`55b01f9`), `gpu_ab.ps1` | 1×, AA Off, 60 FPS | 28.9 % | 17.8 % (1.69 ms) |
| 2026-10-14 | Phase 3 A/B | 2×, AA Off, 60 FPS | 51.3 % / 51.6 % (2× MSAA: 81.4 %) | 26.8 % / 27.4 % (3.30 ms) |
| 2026-10-14 | Phase 3 A/B | 3×, AA Off, 60 FPS | 96.9 %, 59 FPS (2× MSAA: 93.2 %, 33 FPS) | 42.6 %, 60 FPS (5.85 ms) |
| 2026-10-14 | Phase 3 A/B | 2×, AA Off, 30 FPS | 28.3 % (2× MSAA: 41.3 %) | 15.8 % (3.34 ms) |
| 2026-10-14 | Phase 3 A/B | 3×, AA Off, 30 FPS | 50.8 % (2× MSAA: 84.5 %) | 24.4 % (6.08 ms) |
| 2026-10-13 | Phase 0 | 3×3, 2× MSAA, 60 FPS | 90.5 %, 33 FPS (29.1 ms) | |
| 2026-10-13 | Phase 0 | 3×3, AA Off, 60 FPS | 96.9 %, 60 FPS (17.1 ms) | |

The trace and the observer cost nothing when they are off: each hook is one predictable branch. They were
not measured on: they are bring-up tools, not meant to run while playing.

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
9. [Measurements](#measurements)

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
| `sub_822B8120` | `SetShaderGPRAllocation(dev, flags, vertexGprs, pixelGprs)` | traced | 7–8 per frame (48/80 and 16/112 GPRs). Runs three prebuilt command buffers (`INDIRECT_BUFFER`) of 8 one-point draws each, at `0x1FCA1000` (physical). |
| `sub_822BA590`, `sub_822BA818` | Query Issue, GetData | traced | 4 Issue and 1 GetData per frame (the sun flare). |
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
  without the tile hooks. To decide in Phase 3.
- **Sun flare.** The expected sample count scales with the AA level (`PotcaweFlareAaLevelHook`). Host
  queries must count samples the way the game expects. Compare with the Xenos path (patch 0020) at the
  same spot. With tiling, each tile has its own query block (the address is the only state that differs
  per tile).
- **Per-tile resolves.** The engine resolves each tile's rectangle into the same texture under
  SetPredication. With one host target, these become one resolve of the union (the overlap rows are
  resolved twice with the same content).
- **Video planes.** Bink writes its texture planes without Lock/Unlock.

## Phase 2 so far

Runs `potcawe_026`–`046` (`local_docs/05-phase2.md`), 2026-10-13, window 1280×720, default settings (2×
MSAA, 60 FPS).

| Scene | Draw calls per frame | Drawn | Render targets / resolve targets | Shaders / pipelines | Image |
|---|---|---|---|---|---|
| Menus (language, title, main menu) | 2–28 | all | 8 / 20 | 11 / 8 | Language menu: panels, the language list as glyph quads, the selection highlight; no texture art. |
| Port Royal dock | 1,120–1,144 | all | 13 / 33 | 97 / ~180 (growing slowly) | The hill, the buildings and the fog from the default camera; white placeholder textures. |

What the game does that the draw path met:
- **Every draw packet is bin-predicated**, also outside tiled passes. The draw's primitive type, count and
  index buffer are in that packet (`VGT_DRAW_INITIATOR`, `VGT_DMA_BASE`, `VGT_DMA_SIZE`).
- **Index buffers** at the menu are 16-bit with the 8-in-16 swap (`VGT_DMA_SIZE` `0x4000xxxx`); the panels are
  auto-indexed triangle strips.
- **Inline vertex shaders are sometimes overwritten while a level loads.** Vertex shaders are loaded inline
  (`IM_LOAD_IMMEDIATE`, 105–192 dwords) by `DrawIndexedVertices`. While the dock loads, the copy read from
  the command buffer sometimes holds a coherency packet sequence (`COHER_*_HOST`, `WAIT_REG_MEM`, as
  `sub_822B0010` writes it) in the middle of the microcode. It made the SDK's analysis crash the game; now
  those draws are skipped and logged (D20, D21). Reading each segment as it ends made it rarer; the writer
  is not identified yet.
- **Texture addresses.** The resolve destinations' headers hold CPU addresses (`0xA84D2000`); the sampling
  draws' fetch constants hold GPU addresses (`0x084D2000`). Once both were keyed by the GPU address, the
  composite found the scene: 156 texture bindings per frame at the dock are resolve results, the other
  ~9,500 are placeholders until step 7.
- **Uploads.** About 1.5 MB of vertex data in about 170 copies per frame at the dock, most of it data the
  game writes every frame.
- **GPU time.** 10.9 ms per frame at the dock (DiligentCore duration queries), against about 14 ms for the
  Xenos path at 2×2 (four times the pixels). DiligentCore's dynamic heap fills up now and then while the GPU
  falls behind, and stalls the frame.

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

The trace and the observer cost nothing when they are off: each hook is one predictable branch. They were
not measured on: they are bring-up tools, not meant to run while playing.

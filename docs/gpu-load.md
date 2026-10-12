# GPU load with a 2x or 3x render scale

**Status: fixed (2026-10-12).**

## Symptom

With a 2x2 render scale (2560x1440 from the game's 1280x720) the RTX 4070 Ti is at 95-100 % in Port
Royal and the frame takes 20 ms, so the 60 FPS option only reaches 49 FPS; at 3x3, 22 FPS. GRAW, in
comparison, runs 3x3 at 70 %.

## Cause

RenderDoc's per-event GPU timing of the same frame at 1x1 and 2x2 shows that the game's own draws barely
grow with the render scale; what grows are the SDK's own full-screen draws, the *EDRAM ownership transfers*.

The game clears the EDRAM by drawing one rectangle, 19 times per frame, through a surface configured with
a different MSAA mode and pitch than the one it then renders with (4x MSAA at half the pitch has the same
EDRAM tile layout as 2x MSAA at the full pitch: an Xbox 360 trick to clear faster). The SDK keeps a host
render target per (EDRAM base, pitch, MSAA, format), so each clear is another render target aliasing the
same memory: before the clear it copies the current 2x MSAA color and depth into the 4x MSAA alias (so the
alias has the right contents), draws the clear, and copies the cleared contents back when the scene
continues. At 2x2 each copy moves 2560x1440 x 2 or 4 samples of color and depth.

Two things made it worse than needed: the copy *into* the alias is useless (the clear overwrites all of it),
and the SDK assumed the clear rectangle touched the whole EDRAM (it does not run the vertex shader of
unclipped draws on the CPU to measure them, unlike Xenia), so every copy moved all 2048 tiles instead of the
720 rows the game uses. And the copy *back* is useless too: the alias only holds a constant.

## Fix

- SDK patch 0017, option `render_target_skip_transfers_on_overwrite`: when a single triangle, rectangle or
  convex quad covers a rectangle of a render target and the output state replaces every sample (no discard
  or alpha test, depth and stencil always written, all color components written without blending), that
  rectangle is not copied into the render target before the draw. The geometry is checked by running the
  draw's vertex shader on the CPU with the SDK's shader interpreter.
- SDK patch 0018, option `render_target_draw_clears`: when, in addition, what the draw writes is constant
  (the depth from the vertices, the color from running the pixel shader on the CPU with the vertex shader's
  constant outputs), the draw is not done at all: its values are written as clears into the host render
  targets that currently hold that EDRAM, so nothing is copied in either direction and the alias is never
  used.
- `execute_unclipped_draw_vs_on_cpu = true`: the SDK's draw extent estimation runs the vertex shader of
  unclipped draws on the CPU (the Xenia default), so whatever is still copied covers what the draw uses.

All three are in `potcawe.toml` (`src/startup.cpp`; an existing file gets the new settings appended).

Dock of Port Royal, RTX 4070 Ti, same image in every configuration:

| | Before | Patch 0017 | Patches 0017 + 0018 |
|---|---|---|---|
| 2x2, 60 FPS | 49 FPS, 20.4 ms, GPU 99 % | 60 FPS, 16.4 ms, 99 % | 60 FPS, GPU 89 % |
| 2x2, 30 FPS, GPU | 68 % | 53 % | 46 % |
| 3x3, 60 FPS | 22 FPS | 30 FPS | 37 FPS |

What remains at 2x2 is the game's own work plus the SDK's resolves and texture reloads (the game resolves
each tile of the scene several times per frame); see the measurements in the detailed note.

## Going further: no multisampling, one tile (launcher option)

With the copies gone, what remains at 2x2 is the game's own work, and two of its decisions were made for the
console's 10 MB of EDRAM: it renders with 2x MSAA, and because 1280x720 with 2x MSAA does not fit, it draws
the whole scene twice, in two tiles of 416 and 368 rows, resolving each. Measured at 30 FPS and 2x2:
emulating the 2x MSAA as 4x adds 3 ms (so 2x itself costs about that much), and the frame has a fixed part
of ~6.7 ms that does not shrink with the render scale (draws and vertices, half of them the second tile).

The launcher's "Antialiasing: Off (faster)" (`potcawe_no_msaa`, `src/msaa.cpp`) hooks the renderer where
it creates its surfaces (the multisample type comes from a table indexed by its antialiasing level) and
where it computes the number of tiles from that level (ceil(bytes x samples / 10 MB)): the surfaces are
created without multisampling and the count is forced to one, so the scene is drawn once into surfaces
that fit the virtual EDRAM. Dock of Port Royal, 2x2, RTX 4070 Ti: GPU 89 % -> 59 % at 60 FPS (the
multisampling alone: 79 %), 32 % at 30 FPS (was 46 %), 3x3 60 FPS (the cap, GPU 99 %) (was 37 FPS). The image is the same
except for the 2x edge antialiasing, which a 2x or 3x render scale makes much less important. Off by
default, so the first run matches the console.

## Options

| Option (`potcawe.toml` or `--name=value`) | Default | Meaning |
|---|---|---|
| `potcawe_no_msaa` | `false` | `true` renders without 2x MSAA and in one EDRAM tile (launcher: Antialiasing) |
| `render_target_draw_clears` | `true` | `false` draws the clears through their alias render targets and copies the result back (the SDK default) |
| `render_target_skip_transfers_on_overwrite` | `true` | `false` restores the copies into fully overwritten render targets (the SDK default) |
| `execute_unclipped_draw_vs_on_cpu` | `true` | `false` assumes unclipped draws use the whole EDRAM (the SDK default) |

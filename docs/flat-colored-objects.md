# Flat-colored objects in Port Royal

**Status: fixed (2026-10-11).** Six automated runs at 60 FPS with a 2× render scale (the combination that
broke two runs out of three) and one more under RenderDoc (which broke every run) rendered correctly with
the fix.

## Symptom

In the second level (Port Royal, Beckett's dock), buildings, crates, barrels, baskets, palm trunks and
sometimes the characters are drawn in one flat saturated color with a noise pattern, and the palm leaves
come out white and dithered. The color changes with time (pink, cyan, white, purple) and the severity
varies from run to run, from a faint tint over the right texture to full saturation. The dock, the tower,
the sand and the water are fine, and the first level (the prison fortress) never shows it.

It appeared when the GPU was loaded: 60 FPS with a render scale of 2× (the launcher's "Automatic" on a
1440p screen) or more. At 30 FPS, or at 60 FPS with a 1× scale, about one run in three was still faintly
affected; under RenderDoc every run was.

## Cause

The affected objects use the game's lit materials: pixel shaders of 1,000–1,500 instructions once
translated, which read two 3D light volumes and divide by what they read. Everything the game feeds them
(textures in memory, fetch constants, all 256 vertex and pixel shader constants, vertex data) and
everything the host GPU received (constant buffers, samplers, texture contents, system constants) is
byte-for-byte the same in a broken frame and in a clean one, and RenderDoc reproduces the defect when it
replays a broken capture.

RenderDoc's pixel debugger showed what goes wrong inside the translated shader. The SDK (like Xenia)
wraps the whole shader in `loop { switch (pc) { case 0: ... pc = -1; continue; } break; }` to emulate the
Xbox 360 control flow with a `switch`. In the debugger that body ran four times for one pixel, each pass
accumulating more light (the output went from 0.17 to 8.1 and the alpha to 80), and the GPU itself wrote
infinity to those pixels, which the 7e3 render-target format then clamps to its maximum (31.875): the
flat saturated color. How many extra passes happen varies with timing, which is why the severity and the
tint change. Which driver-side behavior turns the `switch` + `continue` form into re-executions on the
RTX 4070 Ti is not known; the translator's other form does not trigger it.

## Fix

`dxbc_switch = false` in `potcawe.toml` (`src/startup.cpp`): the translator emits the control flow as
`if` blocks instead of a `switch` (the form the SDK already uses on Intel GPUs). Shaders are translated
at start-up from the shader storage, so the existing cache does not need to be deleted. No measurable
change in GPU time.

To go back to the SDK's default, set `dxbc_switch = true` in `potcawe.toml` or start the game with
`--dxbc_switch=true`.

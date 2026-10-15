# 60 FPS option

**Status: confirmed (2026-10-11).** A gameplay session at 60 FPS (combat, minigame, cutscene, movement)
showed nothing running at the wrong speed.

## Symptom

The game runs at 30 FPS: one new image every two refreshes of the 60 Hz display.

## Cause

The cap is the game's Direct3D present interval, not its speed. At start-up the renderer sets
`D3DRS_PRESENTINTERVAL` to "every second vblank" (`D3DPRESENT_INTERVAL_TWO`, from a renderer setting whose
value is 2). On every frame the XDK Direct3D library linked into the game encodes that interval into the
swap descriptor; its vblank callback flips the front buffer, and releases the GPU wait inserted before the
next frame, only after that many vblanks have passed since the previous flip. The game's render thread then
waits for the GPU.

The engine itself does not depend on 30 FPS: its clock measures the real time between frames (time base
counter and `KeQueryPerformanceFrequency`), clamps it to 0.2 s and has its own frame limiter set to 1/60 s.
The engine also has an internal "present every vblank" mode (setting value 1) that the game does not use.

## Fix

`src/framerate.cpp` with a `[[midasm_hook]]` in `potcawe_functions.toml`: right after the Direct3D library
reads the render state (in its Swap routine), the hook turns the value 2 into 1 when `potcawe_60fps` is on.
That is exactly what the game would do with the setting at 1: the SDK's vblank stays at 60 Hz, the "present
immediately if late" threshold keeps working, and the game sees half the delta time per frame.

Measured at the title screen with a temporary frame counter: 30.0 frames/s without the option, 60.0 with it.

Choose **Frame rate: 60 FPS** in PiratesAtWorldsEnd Launcher (it writes `potcawe_60fps = true` to
`potcawe_options.toml`) or start the game with `--potcawe_60fps=true`.

## What to watch for

The game's logic uses the measured frame time, so the speed should not change; what can differ at 60 FPS is
anything that advances by a fixed amount per frame rather than per second. Nothing of the kind has been
found in the code or in play so far, but not every part of the game has been seen at 60 FPS. If something is
twice as fast, or an animation, cutscene or minigame drifts, note where it happened and switch back to 30 FPS
in the launcher.

## Options

| Option (`potcawe_options.toml`, `potcawe.toml` or `--name=value`) | Default | Meaning |
|---|---|---|
| `potcawe_60fps` | `false` | `true` presents every vblank (60 FPS); `false` keeps the original 30 FPS |

# Documentation

Short notes on each problem found while making the game run, and what was done about it. Each one gives
the symptom, the cause, the fix and how to turn the fix off, so that you can check it on your own machine.

| Note | Status |
|---|---|
| [cpu-usage.md](cpu-usage.md) — one core at 100 % and a warm CPU while the game runs | Fixed |
| [stutters.md](stutters.md) — short freezes the first time an effect appears | Fixed |
| [frame-rate.md](frame-rate.md) — 60 FPS option (the game is capped at 30 by its present interval) | Confirmed in play |
| [flat-colored-objects.md](flat-colored-objects.md) — lit objects in Port Royal drawn in one saturated color under GPU load | Fixed |
| [gpu-load.md](gpu-load.md) — GPU at 95-100 % with a 2× render scale (the SDK's copies around the game's EDRAM clears) | Fixed |
| [sun-glow.md](sun-glow.md) — the sun's glare at the top of the screen with the sun behind the camera (Port Royal) | Fixed |
| [script-errors.md](script-errors.md) — the game closes after a script error, with faults on several threads | Fixed |
| [quit-prompt.md](quit-prompt.md) — no way to quit the game; B on the title screen now asks whether to quit | Working |

## NativeXRenderer

The renderer's own documents are in [`../native_x_renderer/docs/`](../native_x_renderer/docs/). This folder has the game
side:

| Document | Contents |
|---|---|
| [native-x-renderer-potcawe.md](native-x-renderer-potcawe.md) | POTCAWE's Direct3D map (39 entry points), device layout, engine behavior, frame structure from the trace, the observer's results, the answers to Phase 1's questions, and the measurements |

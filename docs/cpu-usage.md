# CPU usage: one core at 100 %

**Status: fixed (2026-10-11).**

## Symptom

Task Manager shows low overall CPU usage, but one core stays at 100 % and the CPU runs warmer than the
game justifies (60–70 °C on an i7-13700K at the main menu).

## Cause

The game's Direct3D library (part of the Xbox 360 SDK, compiled into the game) waits for the GPU by
spinning: it rereads the command processor's position in a loop until the GPU has caught up. The six wait
loops (ring buffer space, fences, pending swaps, write-backs) all start with `sub_822AF458` and call the
hang check `sub_822AF608` on every iteration. On the console that costs nothing; on a PC it keeps the
game's render thread at 99 % of a core doing no useful work.

Measured from outside the process with a per-thread CPU meter and an instruction-pointer sampler: at the
main menu the whole process used 1.21 cores, the render thread alone 0.99.

## Fix

[`src/gpu_wait.cpp`](../src/gpu_wait.cpp) plus two `[[midasm_hook]]` entries in
[`potcawe_functions.toml`](../potcawe_functions.toml): the first 100 µs of each wait still spin (short
waits behave as before); after that the thread sleeps in 250 µs slices on a high-resolution Windows timer.
Same approach as GRAWRecomp's `src/gpu_wait.cpp`.

Result at the main menu: 1.21 → 0.29 cores; the render thread from 99 % to about 3 %. Frame rate and
behavior unchanged.

## Options

| Option (`potcawe.toml` or `--name=value`) | Default | Meaning |
|---|---|---|
| `potcawe_gpu_wait_spin_us` | 100 | Busy-wait this long before sleeping |
| `potcawe_gpu_wait_sleep_us` | 250 | Sleep per check after that; `0` restores the original busy wait |

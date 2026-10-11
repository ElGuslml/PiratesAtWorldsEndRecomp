# Stutters during play

**Status: fixed (2026-10-11).**

## Symptom

Short freezes while playing, the first time an effect or object appears; they do not come back in the
same place.

## Cause

The SDK compiles a Direct3D 12 pipeline (shader combination + render state) the first time the game uses
it. By default the command processor waits at the end of each submission for the pipelines still being
created, so each new one costs a 50–250 ms frame. The SDK's long-frame log showed exactly that: in the
run with the default settings, the frames the player felt (13:44:45 onwards) had 86–221 ms of pipeline
creation and frame times of 50–133 ms; nothing else (texture loads, uploads, file reads, GPU waits) was
out of the ordinary.

## Fix

SDK patch 0009, option `gpu_async_pipelines_no_frame_wait`, now on by default in `potcawe.toml`
(`src/startup.cpp`): the command processor no longer waits; draws whose pipeline does not exist yet are
skipped, so a new object or effect appears a few frames late instead of freezing the game. In the
verification run the same route created pipelines worth 0.3–1.2 s of compilation per 5 s window while the
longest frame stayed at 35 ms (30 FPS cap); no missing objects or flashes were noticed.

Each pipeline is saved to the shader cache in `userdata\` after its first creation, so a given effect only
costs once per machine.

## Options

| Option (`potcawe.toml` or `--name=value`) | Default | Meaning |
|---|---|---|
| `gpu_async_pipelines_no_frame_wait` | `true` | `false` restores the wait (objects never appear late, but each new pipeline stalls a frame) |

# Skipping the intro videos

**Status: option (2026-11-10), off by default as on the console.** The row "Skip intro videos" under Settings → Game in
PiratesAtWorldsEnd Launcher, or `potcawe_skip_intro`. It works with both renderers.

## What it does

The game starts with two logo videos, Disney Interactive Studios and Eurocom, about 11 seconds before the language
menu. A button press skips each one, but only once the game has loaded its menus (a second or two on a PC), one press
per video. With the option on, nothing has to be pressed: from about a second of the loading screen the game goes
straight to the language menu (at about 4 seconds instead of 14), then to the title screen as usual. The title screen's
video and the levels' videos still play.

## How

`src/intro.cpp`, two `[[midasm_hook]]`s in `potcawe_functions.toml`. It uses the game's own way of skipping a video:

- The game's boot state plays the two videos while it starts loading the menus in the background. Its video player ends
  a video on a button press only if the video is marked skippable, and the boot state marks these two skippable only
  once the menus have loaded.
- With the option on, the boot state sees the menus as loaded as soon as their loading has started, so it marks both
  videos skippable with its own code. The video player then sees a button pressed for those two videos only (found by
  their file IDs, `EB000002` and `EB000001`), and ends them as if the player had pressed one.
- The boot state then takes its usual path: its loading screen until the menus have loaded, the language menu, the
  title screen. It is not done before the loading starts: with no video left at that point the boot state skips the
  language menu too.

## Turning it on

Choose it in the launcher, which writes it to `potcawe_options.toml`, or start the game with `--potcawe_skip_intro=true`.

# Achievements

**Status: working (2026-10-22).** Tested at Beckett's Dock with both renderers (NativeXRenderer on Vulkan, Xenos on
Direct3D 12): an unlock's notification, the F7 list, and the launcher's Achievements page.

## Symptom

The game's 30 achievements (1000G) already unlocked and were saved, but:
- the notification was a small box at the bottom right in a 10-pixel font, always in English ("Achievement
  Unlocked"), and without the achievement's icon with NativeXRenderer;
- F7's list had no icons with NativeXRenderer either, and with the Xenos path it made the GPU redraw the screen at
  the monitor's refresh rate (220 times a second) while it was open;
- the launcher could not show them: they are inside the game's executable, which is compressed.

## Fix

All of it is in the ReXGlue SDK (changes 37 and 38 of its `recomp` branch), so the other games on it get it too:
- **The notification** is a card at the top center of the screen, as on the console: the achievement's icon,
  "ACHIEVEMENT UNLOCKED" in your language, its name and description, its gamerscore, and a bar for the time left. It
  slides in and stays 6 seconds. It is drawn in Barlow, the launcher's font, at your screen's resolution.
- **F7** opens the list of every achievement: its icon (dimmed while locked), name, description, gamerscore and the
  date you unlocked it, with your totals and progress. F7 closes it. It is drawn with the game's frames.
- **The icons** are the game's own, and both renderers draw them.
- **For the launcher**, the game writes the achievements' list in every language it has, and their icons, next to
  your unlocks in `userdata\achievements\`. The launcher's Achievements page and the Play page's card read them. If
  the list is missing (the first time), the launcher asks the game to write it without starting it (about a second).

The game asks for its language in its own first menu, so these texts use the Windows display language, as the
launcher and the quit prompt do (English, French, German, Italian or Spanish; the others get English).

## Files

| File (in `userdata\achievements\`) | What it holds |
|---|---|
| `425607D1.toml` | Your unlocks and when (written by the game) |
| `425607D1.info.toml` | The achievements in every language, for the launcher |
| `425607D1\*.png` | Their icons |

Deleting `425607D1.toml` locks every achievement again; the others are written again by the game.

## Options

| Option (`potcawe.toml` or `--name=value`) | Default | Meaning |
|---|---|---|
| `bind_achievements` | `F7` | The key that opens and closes the list |
| `achievements_cache_only` | `false` | `true`: write the achievements' list for the launcher and quit without starting the game |

## What to watch for

An unlock without its notification, a notification in the wrong language, an icon missing, or the launcher's page
empty after the game has run once.

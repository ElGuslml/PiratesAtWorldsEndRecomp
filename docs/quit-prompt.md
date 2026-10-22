# Quitting from the title screen

**Status: working (2026-10-21).** Tested on the title screen and in Port Royal with both renderers (NativeXRenderer
on Vulkan, Xenos on Direct3D 12), with the keyboard as the controller.

## Symptom

The game has no way to quit it: on the Xbox 360 that was the console's guide button. On PC the only way out was to
close the window.

## Fix

On the title screen ("PRESS START"), **B** now asks whether to quit: **A** (or Enter) quits, **B** (or Escape) goes
back to the title screen. The mouse works too. The dialog is not drawn with the game's graphics: it is a window over
the dimmed game, like the SDK's other dialogs. The game does not see the button presses that open, answer or close
it.

B anywhere else does what it always did: on the language menu, the autosave notice and in the game nothing opens, and
B in the main menu still goes back to the title screen (only a new press of B there opens the dialog).

The dialog belongs to the SDK (`rex/ui/overlay/quit_prompt.h`, changes 34 and 35 of its `recomp` branch), so other
games can use it; KameoRecomp has the same. This game's side is `src/quit_prompt.cpp`. The game's menus are GX menu
nodes, and two events of its menu code say which one is on screen:
- a node set up from its widget: the language menu, the title screen (widget `C4300007`), the main menu;
- a node linked to the next menu during a transition: the title to the autosave notice, the notice to the main menu,
  the main menu back to the title.

Hooks in `potcawe_functions.toml` record both. The SDK asks, when B is pressed, whether the menu on screen is the
title screen.

The game asks for its language in its own first menu, so the dialog uses the Windows display language (English,
French, German, Italian or Spanish; Dutch and the others get English).

## Turning it off

Start the game with `--quit_prompt=false`, or add `quit_prompt = false` to `potcawe.toml`. B on the title screen then
does what it always did (nothing).

| Option (`potcawe.toml` or `--name=value`) | Default | Meaning |
|---|---|---|
| `quit_prompt` | `true` | `true`: B on the title screen asks whether to quit the game |

## What to watch for

The dialog opening anywhere but the title screen, not opening there, a B or A press that reaches the game after the
dialog closes, or a controller that stops responding after it.

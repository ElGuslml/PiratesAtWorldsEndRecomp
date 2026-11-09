# Button prompts for a PlayStation controller

**Status: working (2026-11-09), tested in play with a DualSense.** With a PlayStation controller in hand,
the game shows the PlayStation's buttons (cross, circle, square, triangle, L1, R2...) instead of the Xbox 360's, in
its texts and on its HUD. Any other controller, or the keyboard, gets the original art back. Both renderers.

## What the game has

The game's data is shared with its PlayStation 3 version, and the Xbox 360 disc still carries the PS3's button art,
unused:

- **In text**, a button is a character of the game's fonts. A text such as "Seleccionar - ##ABUTT" has a macro that the
  game expands into character 0x13, which the font draws as the green A. The three fonts (one 512x512 atlas each in
  `menuresource.edb`, and the glyph tables `font.fnt`, `font2.fnt`, `font3.fnt`) have 28 such characters, codes 1 to
  30, and the PS3's 28 at codes 0x100 to 0x11B, in the same order: 0x110 is the cross, 0x115 L1, 0x119 START.
- **On the HUD**, the buttons are sprites of 128x128, one texture each, in `hud_main.edb`: A B X Y, the shoulders and
  triggers, START, BACK, the d-pad, the guide button, the sticks with their arrows. Their PS3 counterparts are in the
  same file, of the same size and format.

## How

`src/button_prompts.cpp`, two `[[midasm_hook]]`s in `potcawe_functions.toml`, and the SDK's change 77
(`rex::input::GetActiveDevice`: the controller that answers for player 1 now, with SDL's type of controller).

- **Which art.** Every fourth frame (the Direct3D library's Swap, `sub_822B58B0`) the game asks the SDK which device
  player 1 last used. SDL's types PS3, PS4 and PS5 give the PlayStation art; anything else, the Xbox art. Through Steam
  Input, SDL still reports the controller behind Steam's virtual pad.
- **Texts.** The game reads a glyph's record (its place in the atlas, its size and advance) from the font's table as it
  lays out each line. The 28 Xbox records of each loaded font get the PS3 records; the original records are kept and
  put back for another controller. No text changes, only the pictures of these characters. L1, L2, R1 and R2 are wider
  than LB, LT, RB and RT, and the lines lay out with their widths.
- **Sprites.** After the game decompresses a section of a package (`sub_82324F80`), a texture that holds one of the 26
  Xbox sprites (compared with the player's `hud_main.edb`, read at start) is noted, and gets its PS3 counterpart's
  pixels while the PlayStation art is shown. When the controller changes, every noted texture that still holds either
  version is rewritten. Both renderers watch the texture memory and take the new pixels as they would the game's.

## Options

`potcawe_button_prompts` in `potcawe.toml`, or on the command line:

| Value | Buttons shown |
|---|---|
| `auto` (default) | The PlayStation's with a PlayStation controller in hand, the Xbox 360's otherwise |
| `xbox` | Always the Xbox 360's, as the original |
| `playstation` | Always the PlayStation's |

The log says which art is shown and why (`[prompts]`), and when the HUD's sprites load.

## Not covered yet

- Words in the texts: the title's "Press START" (the PS3 also said START), and a few legends written with letters
  ("A - Select"). The autosave notice still names the Xbox 360's memory unit.
- The controls page's diagram: the disc has the Sixaxis', but its left stick and d-pad are where the 360's labels
  for the d-pad and the left stick are, so the labels would point at the wrong parts.
- Four 64x64 A B X Y sprites in `hud_main.edb` (dimmed), which have no PS3 counterpart.
- Other controllers' own art (Nintendo, Xbox One's menu buttons, the keyboard's keys): the disc has none; it would need
  icons of our own.

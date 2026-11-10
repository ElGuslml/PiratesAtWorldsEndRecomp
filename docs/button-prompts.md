# Button prompts for a PlayStation controller

**Status: working (2026-11-09).** With a PlayStation controller in hand, the game shows the PlayStation's buttons
(cross, circle, square, triangle, L1, R2...) instead of the Xbox 360's, in its texts and on its HUD. A PS4 or PS5
controller gets its own OPTIONS and SHARE / CREATE buttons. Any other controller, or the keyboard, gets the original
art back. Both renderers. The PS3 art was tested in play with a DualSense; the PS4 and PS5 buttons are new.

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

The PS4's and the PS5's buttons are the PS3's but for two: OPTIONS and SHARE (PS4) or CREATE (PS5) where the PS3 has
START and SELECT. The disc has no picture of those; ours are in `res/prompts/`, retouched from Xelu's Free Controller &
Key Prompts (CC0) by `res/prompts/make_prompts.py`: cropped, scaled, outlined in dark, the PS4's made light, laid out
for a line of text, and embedded in the executable.

## How

`src/button_prompts.cpp`, three `[[midasm_hook]]`s in `potcawe_functions.toml`, `src/dxt5_texture.cpp`, and the SDK's
change 77 (`rex::input::GetActiveDevice`: the controller that answers for player 1 now, with SDL's type of
controller).

- **Which art.** Every fourth frame (the Direct3D library's Swap, `sub_822B58B0`) the game asks the SDK which device
  player 1 last used. SDL's types PS3, PS4 and PS5 give the PlayStation art; anything else, the Xbox art. Through Steam
  Input, SDL still reports the controller behind Steam's virtual pad.
- **Texts.** The game reads a glyph's record (its place in the atlas, its size and advance) from the font's table as it
  lays out each line. The 28 Xbox records of each loaded font get the PS3 records; the original records are kept and
  put back for another controller. No text changes, only the pictures of these characters. L1, L2, R1 and R2 are wider
  than LB, LT, RB and RT, and the lines lay out with their widths.
- **Our glyphs.** When the game decompresses a font atlas, our four pictures are drawn into its empty rows (from row
  320; the game's glyphs end at row 313), as DXT5 blocks in the atlas's tiled layout, with its mips. For a PS4 or PS5
  controller, START's and BACK's records point at them, with their own width.
- **Sprites.** After the game decompresses a section of a package (`sub_82324F80`), a texture that holds one of the 26
  Xbox sprites (compared with the player's `hud_main.edb`, read at start) is noted, and gets the pixels of the style in
  use: the PS3 counterpart's, or for START and BACK with a PS4 or PS5 controller our picture, encoded at start. When the
  controller changes, every noted texture that still holds one of its versions is rewritten. Both renderers watch the
  texture memory and take the new pixels as they would the game's.
- **The word START.** Two texts name the button with the word, in all six languages: the title's "Press START" and
  the co-op's "Press START to join". The game finds a text by its GUID (`sub_82348D28`); a hook after the look-up gives
  these two a copy with the controller's own word: OPTIONS (PS4, PS5), MENU (Xbox One, Series), + (Switch), or the key
  bound to Start with the keyboard (X with the default keys). The PS3 and the Xbox 360 say START. A text takes the word
  when the game sets it up (the title, each time it is shown). The original text is not changed.

## Options

`potcawe_button_prompts` in `potcawe.toml`, or on the command line:

| Value | Buttons shown |
|---|---|
| `auto` (default) | For the controller in hand: the PS3's, PS4's or PS5's with a PlayStation controller, the Xbox 360's otherwise; START in the controller's own word |
| `xbox` | Always the Xbox 360's, and START, as the original |
| `ps3` (or `playstation`) | Always the PS3's, and START |
| `ps4`, `ps5` | Always the PS4's or the PS5's, and OPTIONS |

The log says which art is shown and why (`[prompts]`), when the font atlases get our glyphs and when the HUD's
sprites load.

## Not covered yet

- The texts that name the Xbox 360 (the autosave notice's memory unit, Xbox Live, the Guide button).
- The controls page's diagram: the disc has the Sixaxis', but its left stick and d-pad are where the 360's labels
  for the d-pad and the left stick are, so the labels would point at the wrong parts.
- Other controllers' own art (Nintendo, the Xbox One's MENU and VIEW, the keyboard's keys): the same way as the PS4's
  and the PS5's, with pictures of our own.

Not needed: three legends written with letters ("A - Select     B - Backup", "A - Continue", "Start - Continue
A - Select") and four 64x64 A B X Y sprites in `hud_main.edb` (a flatter drawing, with no PS3 counterpart) are used
nowhere: no package, script or code refers to them.

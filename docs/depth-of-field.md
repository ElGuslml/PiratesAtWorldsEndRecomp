# A lattice in the depth of field at high resolutions

**Status: cause found (2026-11-10), not fixed yet.** Turning the depth of field off (Settings → Graphics → Screen
effects) removes it.

## Symptom

With NativeXRenderer above 720 lines (1440p, ultrawide), what the depth of field blurs, the distant town and trees at
Port Royal, shows a fine diagonal crosshatch, as if the image were seen through a screen door. Turning the other screen
effects on and off does not change it; turning the depth of field off removes it. It looks the same whether or not the
reduced copies of the screen are kept at the game's resolution (NativeXRenderer's D127, below), so it is not the cause
of the bloom lattice fixed in GH Metallica.

## Cause

The game's depth of field has two modes, chosen by its setting "DoF Use Strong Blur":

- **The usual one** (Port Royal's dock): one full-screen pass whose shader takes the sharp scene at full size, and for
  the blurred part averages it with four samples one texel away diagonally, the distance given by the game as
  1/1280 and 1/720 of the screen. At 1280x720 those are the neighboring pixels and the result is a soft blur. At a higher
  resolution the same distance is two or three pixels: the four samples skip the pixels between them, and every detail
  appears as four faint diagonal copies, the crosshatch. The Xenos renderer shows the same at a resolution scale above 1.
- **"Strong Blur"**: the scene reduced to 320x180, blurred, and blended back. NativeXRenderer keeps that reduced copy at
  the game's resolution (D127, set for this game since 2026-11-10, as the bloom's chain from 320x180 down), so it shows
  no lattice. It blurs far more than the usual mode, so using it everywhere would change the game's look.

## What a fix needs

Four samples cannot cover two or three pixels each. The four diagonal samples have to read a copy of the scene at the
game's resolution, each of its pixels the average of the high resolution pixels it covers, while the sharp center
keeps the full resolution. That is a change to NativeXRenderer.

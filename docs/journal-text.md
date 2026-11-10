# The journal's first page: text with light halos

**Status: fixed with NativeXRenderer (2026-11-10, its D137), on by default.** The Xenos renderer still shows it.

## Symptom

In the journal (Back), the first page of the section that opens, the objective beside the map, had odd text: a hard,
light, jagged outline around every letter. The following pages looked right, with a soft dark shadow under the letters.
Reported by a player; seen at every resolution, with both renderers.

## Cause

Each page is drawn as 2D art (the parchment, the decorations, the text with its drop shadow) into a surface, copied to a
texture, and that texture is blended over the 3D book by its alpha. The game draws the first page into its HDR surface,
whose format (`2_10_10_10_FLOAT`) keeps only 2 bits of alpha, and the second into an 8-bit surface. The letters' soft
shadow leaves alpha a little under 1; at 2 bits it is rounded to steps of a third, and where it drops, the book's blank
parchment shows through the page: the light, jagged rings. The 8-bit page keeps the same alpha smooth, so its shadow
reads as a shadow.

Both renderers keep those 2 bits when the surface is copied, as the console's video memory stores them, so the original
game most likely looks the same.

## Fix

NativeXRenderer keeps the alpha of that format at full precision when it copies it, only within 0 to 1, for this game
(`float_7e3_full_alpha` in `src/native_x_renderer_potcawe.cpp`). The first page now looks like the others. Everything
else the game draws through that surface looked the same in the tests.

## Turning it off

`native_x_renderer_float_7e3_full_alpha = false` in `potcawe.toml` (read at startup).

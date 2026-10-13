# Sun glare where there is no sun

**Status: fixed (2026-10-13).** Automated runs at the dock of Port Royal (camera turned toward the sun and
away from it, with and without 2× MSAA, 2× and 3× render scale) show the glare only toward the sun, with the
same strength with and without antialiasing.

## Symptom

In the second level (Port Royal, Beckett's dock) the sun's glare washes out the top of the screen also when
the camera looks away from the sun (the sun is behind the camera), and toward the sun it is much stronger
than it should be, including through the tower.

## Cause

The game draws a small quad at the sun's position inside an occlusion query and sets the strength of the
glare from the fraction of its samples that passed the depth test: none when the sun is behind the camera,
off screen or behind a wall.

The SDK runs these queries on the host GPU, but it expected the begin and the end of a query to point at the
same memory. The game's Direct3D library points them at the two halves of one block (the result is end minus
begin), and with 2× MSAA, where the scene is drawn in two tiles with one block each, it marks only the last
tile's end. At the first query the SDK took this for an error, switched host queries off for the rest of the
session and answered every query with a fixed 1000 samples: the sun always counted as fully visible.

## Fix

- SDK patch 0020: the end of a query pairs with the begin whatever its address, any event while the game's
  query is open ends it (the first tile), the begin counts are zero, and the two counts the game adds are
  written together so that it never reads half a result.
- `src/msaa.cpp`: with "Antialiasing: Off" the game still believes it renders with 2× MSAA and expects two
  samples per pixel from the query, so the glare would reach only half its strength; the flare is told
  there is no antialiasing.

Cost: the SDK waits for the GPU to finish everything up to each query before it writes the result (one query
per frame, two with 2× MSAA). With the GPU already at 100 % (3× render scale with 2× MSAA on an RTX 4070 Ti)
that is about 2 ms per frame, 42 → 37 FPS; at 2× it stays at 60 FPS.

To go back to the old behavior (the sun always fully visible, no wait), start the game with
`--occlusion_query_enable=false`.

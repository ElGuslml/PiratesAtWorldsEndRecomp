# Turning off the game's screen effects

**Status: options (2026-11-06), all on by default:** rows under Settings → Graphics → Screen effects in
PiratesAtWorldsEnd Launcher, or the options below. They work with both renderers.

## What the game applies

After a view's 3D the engine goes through a list of post-processing effects. Levels, cameras and timeline nodes turn
them on and set their parameters as the scene goes on.

| Option | Effect | Seen at the Port Royal dock |
|---|---|---|
| `potcawe_fx_bloom` | The glow around the sun, fire and bright surfaces | Yes: off, bright pixels lose about 5.7 of 255 and dark ones about 0.3 |
| `potcawe_fx_depth_of_field` | What is far from the focus, blurred | Yes: off, the distant town is about 70 % sharper and what is near does not change |
| `potcawe_fx_color` | The saturation, contrast, hue and temperature each place sets | Yes: off, the image is bluer and more saturated |
| `potcawe_fx_heat_haze` | The image shimmering as through hot air | No: it does not run there |
| `potcawe_fx_radial_blur` | The blur outward from the center of the image | No: it does not run there |
| `potcawe_fx_motion_blur` | The blur of the camera's fast moves | No: it does not run there |

The last three use the same mechanism as the depth of field, which is the one that was seen working. Which levels or
moments use them is not known yet: their data is in the game's packed files.

The game has no vignette and no film grain: neither a shader nor a setting for them exists. Its own fullscreen
antialiasing is covered by the launcher's Antialiasing row. The sun's glare is not one of these effects
([sun-glow.md](sun-glow.md)).

## How

`src/screen_fx.cpp`, four `[[midasm_hook]]`s in `potcawe_functions.toml`.

- **Depth of field, heat haze, radial blur, motion blur.** The post-processing (`sub_823D2160`) runs an effect when its
  mask (`[effect+8]`) has a bit in common with the view's flags, which it reads into `r26` and uses for nothing else.
  The hook takes the bits of the effects turned off out of `r26`: 1, 8, 16 and 32. It is the same as the view not asking
  for them, a path the engine already takes, and an effect turned back on works from the next frame.
- **Bloom.** It has no bit: bit 2 is the whole HDR effect, whose final pass (`sub_8245D4A0`) draws the scene into the
  output, so that bit cannot be taken out. Bloom, the eye's adaptation, the tone mapping and the color adjustment are
  parts of that pass. The bloom's blurred copy is added with the setting "HDR Bloom Colour", which the pass loads into
  pixel constant c19; the hook gives it black instead. The setting itself is not changed. The passes that prepare the
  blurred copy still run, so this changes the image, not the GPU's work.
- **Color adjustment.** "Colour Adjust Enable" (`effect+176`) picks the final shader: `HDRFinalPass_CA`, with the color
  matrix, or `HDRFinalPass`. `sub_8245DCD0` does the same for `HDREngineToSRGB`. The hooks make both see it off.

## Turning one off

Choose it in the launcher, which writes it to `potcawe_options.toml`, or start the game with for example
`--potcawe_fx_bloom=false`.

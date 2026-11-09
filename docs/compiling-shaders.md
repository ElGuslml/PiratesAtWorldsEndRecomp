# Stutters the first time something is shown

**Symptom.** The first time the game shows something (a level, a character, an effect), it freezes for a moment, or
objects appear a few frames late. The next time it plays smoothly. On a new install, or after a graphics driver update,
arriving at the Port Royal dock froze for about 1.5 s in all (one freeze of 0.5-0.7 s), and thousands of draws were
missing during the first frames.

**Cause.** NativeXRenderer translates the game's shaders to Vulkan, and the graphics driver compiles each combination of
them with the drawing state (a pipeline) the first time a draw needs it. The renderer keeps every pipeline it compiled
(`userdata\cache\native_x_renderer\425607D1.pipelines`) and prepares them at the next start, so only a first time
stutters, but every first time does: each level brings its own shaders.

**Fix (NativeXRenderer D129, D136; 2026-11-09).** The game ships a list of the pipelines found while playing it,
`425607D1.pipelines.list` beside `PiratesAtWorldsEndRecomp.exe`. It names each shader by a hash of its code: the code
itself comes from the player's own game files, so nothing of the game is in the list. The game keeps its shaders inside
its level packages (`Data\FileList.000` and `.001`), compressed; `src/level_packages.cpp` reads them
([native-x-renderer-potcawe.md](native-x-renderer-potcawe.md#the-shaders-in-the-games-files)). The vertex shaders are
rebuilt as the game's Direct3D library changes them: their vertex inputs set for the vertex layout they draw with, and
their outputs linked to the pixel shader they draw with.

The first time Play is pressed, the launcher shows "Compiling shaders" while the game's executable, without a window,
rebuilds those shaders, compiles every listed pipeline and the ones stored before for this graphics card and driver,
and saves the driver's binaries. The game then creates them at its start. It shows again only after an update of the
game or of the graphics driver. On NVIDIA GPUs a pipeline the list does not name, but whose shaders the game drew
before, is linked from parts in microseconds (Vulkan's graphics pipeline libraries) while its optimized version compiles
in the background.

**The list** (from Gustavo's play, 2026-11-09): 225 of 225 shaders (109 pixel, 116 vertex), 253 translations and 327
pipelines, 77 KB. It covers what that play reached; a level beyond it compiles its new pipelines at its draws, as
before, and they are kept for the next start. A list from more play (several players' storages can be merged) covers
more.

**How to turn it off.** Skip on that screen starts the game at once; it then compiles at its draws, as before. Without the
list file, or with the Xenos renderer, the launcher does not show it. Started without the launcher, the game does what it
did before. `native_x_renderer_pipeline_library = off` (in `potcawe.toml`) compiles every pipeline whole. The tool can be
run by hand: `PiratesAtWorldsEndRecomp.exe --native_x_renderer_prepare_pipelines=progress.txt`.

**For developers.** A new list from stored pipelines (each a `425607D1.pipelines` file, read without being changed):
`PiratesAtWorldsEndRecomp.exe --native_x_renderer_make_pipeline_list=tools\pipelines\425607D1.pipelines.list
--native_x_renderer_pipeline_list_inputs=<a storage>|<another>`. Its log line says how many shaders the game's files
had: every one should be found. The list stays out of git (`tools\pipelines\`); `scripts\package-release.ps1` puts it
in the package.

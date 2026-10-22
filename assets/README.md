# Game files

Put the files of your own copy of **Pirates of the Caribbean: At World's End (Xbox 360)** here. This project
does not provide them: they come from your own disc, or from an image you made of it. This file must exist:

```
assets/default.xex
```

The whole disc content goes in this folder. `PiratesAtWorldsEnd Launcher.exe` extracts it from your ISO when it
starts without it (a full Redump image or a trimmed one). You can also extract it yourself, for example with
[extract-xiso](https://github.com/XboxDev/extract-xiso/releases).

The build needs `default.xex` (the code is recompiled from it); the game needs everything else.
The expected executable has CRC32 `A31B1069` (6,041,600 bytes, title ID `425607D1`);
`PiratesAtWorldsEndRecomp.exe` warns at startup if yours is different.

Nothing in this folder is tracked by git.

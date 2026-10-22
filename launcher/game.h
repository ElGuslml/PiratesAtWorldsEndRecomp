// What the launcher knows about this game: its name, its executables and the disc it is recompiled from.
// The rest of the launcher's first run (iso.*, install.*, page_install.cpp) is the same for any game.

#pragma once

#include <cstdint>

namespace launcher::game {

inline constexpr const char* kName = "Pirates of the Caribbean: At World's End";
// The recompiled executable, next to the launcher.
inline constexpr const char* kRecompExecutable = "PiratesAtWorldsEndRecomp.exe";
// The disc's executable, at its root and in assets\ once extracted; extracted last, so that its presence marks a
// complete installation.
inline constexpr const char* kDiscExecutable = "default.xex";
// The disc this build was recompiled from (src/startup.cpp checks the same CRC at the game's start).
inline constexpr uint32_t kTitleId = 0x425607D1;
inline constexpr uint32_t kExecutableCrc32 = 0xA31B1069;
inline constexpr const char* kRegion = "USA";
// The game partition's files on that disc (25 files): the space the first run asks for before an ISO is chosen.
inline constexpr uint64_t kInstalledBytes = 6835718080;

// Where this project's releases are (updater.cpp): GitHub's API for them, newest first, and their page.
inline constexpr const char* kReleasesApi =
    "https://api.github.com/repos/ElGuslml/PiratesAtWorldsEndRecomp/releases?per_page=20";
inline constexpr const char* kReleasesPage = "https://github.com/ElGuslml/PiratesAtWorldsEndRecomp/releases";
// The launcher, which the updater replaces while it runs (renamed, then started again).
inline constexpr const char* kLauncherExecutable = "PiratesAtWorldsEnd Launcher.exe";

}  // namespace launcher::game

// potcawe - the game's compiled shaders in its level packages, for NativeXRenderer's pipeline list (its D129, D136).
//
// The game keeps its files in two archives, Data/FileList.000 and .001, indexed by Data/FileList.bin (Eurocom's engine).
// Its shaders are not in a file of their own: each GEOM package (a level, a character, an object) carries the shaders it
// draws with, as XDK shader containers, most of them compressed. NativeXRenderer's list names shaders by hash; this
// reader gives it those containers, decompressed, from the player's copy of the game (docs/native-x-renderer-potcawe.md,
// "The shaders in the game's files").

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <vector>

namespace potcawe {

// Every buffer of the game's packages that holds shaders, decompressed, given to `add` in turn (NativeXRenderer's
// Profile::read_shader_archives).
void ReadPackagedShaders(const std::filesystem::path& game_data_root,
                         const std::function<void(const uint8_t* data, size_t size)>& add);

// The textures of the GEOM package whose index id is `package_id` (hud_main.edb: 0xC10001EE) that have one of `guids`,
// by GUID, each decoded as the game has it in memory once loaded (level_packages.cpp, pair 3). Those that cannot be read
// are left out.
std::unordered_map<uint32_t, std::vector<uint8_t>> ReadPackageTextures(const std::filesystem::path& game_data_root,
                                                                       uint32_t package_id,
                                                                       const std::vector<uint32_t>& guids);

// One Eurocom EDL block (its 12-byte header included, `size` bytes at most) decoded into `out`. False if it is damaged,
// or little-endian, or of method 1 (the game's shaders use 2, and 0 for data stored as it is).
bool DecodeEdl(const uint8_t* block, size_t size, std::vector<uint8_t>& out);

}  // namespace potcawe

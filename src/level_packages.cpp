// potcawe - see level_packages.h.
//
// Data/FileList.bin: a 0x14-byte header (the entry count at +8), then 0x24-byte big-endian entries {the header's size
// for a GEOM package, else the size; id; type; 0; size; 1; hash; offset; part}, the part 0 for FileList.000, 1 for .001.
// The shaders are in two places:
// - Every type 0x27A0 entry is a GEOM package. Its header ("GEOM", the header's size at +0x18) has 14 (count, offset)
//   pairs at +0x30, each offset counted from its own field; pair 2 lists the package's shader records, 7 dwords each:
//   {0x4F << 24 | index, hash, 0, index, offset in the package, stored size, size}. A record is an EDL block (EDL\x82:
//   big-endian, method 2) of its stored size that decodes to its size, or its data as it is when the size is 0. In
//   Gustavo's copy: 751 records in the 622 packages (one stored as it is), 29.9 MB that decode to 72.3 MB.
// - The one type 15 entry (id 0x9D000000, 37 KB in FileList.001) holds 52 containers as they are, outside the packages
//   (33 of the stored shaders are only there).
// Gustavo's 225 stored shaders (local_docs/24-cold-start-pipelines.md) are all there: the 109 pixel shaders as the
// files have them, the 116 vertex shaders with their fetches and exports patched by the D3D library.
// Pair 3 lists a package's textures in records of the same shape: {class << 24 | index (0x42 or 0x02), GUID, 0, index,
// offset, stored size, size}. A texture decodes to what the game keeps in memory: a 4 KB header (0xAF000008, the
// width and height at +0x24, the format at +0x2F: 1 DXT5, 4 DXT1), then the tiled surface and its mips.

#include "level_packages.h"

#include <rex/logging.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>

namespace potcawe {

namespace {

uint32_t Be32(const uint8_t* p) {
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | uint32_t(p[3]);
}

constexpr size_t kIndexHeader = 0x14, kEntrySize = 0x24;
constexpr uint32_t kGeomType = 0x27A0;
constexpr uint32_t kShaderEntryType = 15;
constexpr size_t kGeomHeaderSizeField = 0x18, kGeomPairs = 0x30, kGeomPairCount = 14, kShaderPair = 2, kTexturePair = 3;
constexpr size_t kRecordSize = 28;
constexpr uint32_t kShaderClass = 0x4F;

// The bits of a method 2 block, read from 32-bit big-endian words, least significant bit first (the game keeps the
// current word and the next one in registers; this is the same order).
class BitReader {
 public:
  BitReader(const uint8_t* data, const uint8_t* end) : next_(data), end_(end) {}

  uint32_t Get(int count) {
    while (available_ < count) {
      uint32_t word = 0;
      if (end_ - next_ >= 4) {
        word = Be32(next_);
        next_ += 4;
      } else {
        overrun_ = true;
      }
      bits_ |= uint64_t(word) << available_;
      available_ += 32;
    }
    const uint32_t value = uint32_t(bits_ & ((uint64_t(1) << count) - 1));
    bits_ >>= count;
    available_ -= count;
    return value;
  }
  bool overrun() const { return overrun_; }

 private:
  const uint8_t* next_;
  const uint8_t* end_;
  uint64_t bits_ = 0;
  int available_ = 0;
  bool overrun_ = false;
};

// Data/FileList.bin, read whole. False (and a warning naming `what`) when it is missing or not the game's index.
bool LoadIndex(const std::filesystem::path& folder, const char* what, std::vector<uint8_t>& index, uint32_t& count) {
  std::ifstream file(folder / "FileList.bin", std::ios::binary);
  if (!file) {
    REXLOG_WARN("[potcawe] {}: {} is missing", what, (folder / "FileList.bin").string());
    return false;
  }
  index.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
  count = index.size() >= kIndexHeader ? Be32(index.data() + 8) : 0;
  if (!count || kIndexHeader + uint64_t(count) * kEntrySize > index.size()) {
    REXLOG_WARN("[potcawe] {}: FileList.bin is not the game's index", what);
    return false;
  }
  return true;
}

// FileList.000 and .001, read by an index entry's part and offset.
class Archives {
 public:
  explicit Archives(const std::filesystem::path& folder)
      : parts_{std::ifstream(folder / "FileList.000", std::ios::binary),
               std::ifstream(folder / "FileList.001", std::ios::binary)} {}

  bool Read(uint32_t part, uint64_t offset, size_t size, std::vector<uint8_t>& out) {
    if (part > 1 || !parts_[part].is_open()) return false;
    out.resize(size);
    parts_[part].clear();
    parts_[part].seekg(std::streamoff(offset));
    parts_[part].read(reinterpret_cast<char*>(out.data()), std::streamsize(size));
    return parts_[part].gcount() == std::streamsize(size);
  }

 private:
  std::ifstream parts_[2];
};

}  // namespace

// Eurocom's EDL compression as the game decodes it (the header: sub_8240E438; method 2: sub_8240DB30). A block: "EDL", a
// byte (bit 7 big-endian, the low bits the method: 0 stored, 1 and 2 two kinds of LZ), the block's stored size (the
// header included) and the size it decodes to. Method 2 is an LZ77 of codes read by BitReader:
//   0, 8 bits             a literal byte
//   1 1 0, 8 bits d       2 bytes from distance d + 1
//   1 1 1 0, a distance   3 bytes
//   1 1 1 1, 8 bits n     n + 8 bytes (9 to 263) and a distance; n = 0 ends the block
//   1 0 b 0, a distance   4 + b bytes
//   1 0 b 1 c             (3 + b) * 2 + c bytes (6 to 8) and a distance; 9: a run of (4 bits + 3) * 4 literal bytes
// A distance: its high part (0 to 15) by a prefix, then its low 8 bits: (high << 8) + low + 1, up to 4096.
bool DecodeEdl(const uint8_t* block, size_t size, std::vector<uint8_t>& out) {
  out.clear();
  if (size < 12 || std::memcmp(block, "EDL", 3) || !(block[3] & 0x80)) return false;
  const uint32_t method = block[3] & 0x7F;
  const uint32_t stored = Be32(block + 4), decoded = Be32(block + 8);
  if (stored < 12 || stored > size || decoded > (256u << 20)) return false;
  out.resize(decoded);
  if (method == 0) {
    if (stored - 12 < decoded) return false;
    std::memcpy(out.data(), block + 12, decoded);
    return true;
  }
  if (method != 2) return false;

  BitReader bits(block + 12, block + stored);
  size_t at = 0;
  auto distance = [&bits]() {
    uint32_t high = 0;
    if (bits.Get(1)) {
      const uint32_t first = bits.Get(1);
      if (!bits.Get(1)) {
        high = first ? 1 : 2 + bits.Get(1);
      } else {
        high = ((first + 2) << 1) + bits.Get(1);
        if (!bits.Get(1)) high = (high << 1) + bits.Get(1);
      }
    }
    return (high << 8) + bits.Get(8) + 1;
  };
  auto copy = [&out, &at, decoded](uint32_t length, uint32_t from) {
    if (from > at || length > decoded - at) return false;
    for (uint32_t i = 0; i < length; ++i, ++at) out[at] = out[at - from];
    return true;
  };
  for (;;) {
    if (bits.overrun()) return false;
    if (!bits.Get(1)) {
      if (at >= decoded) return false;
      out[at++] = uint8_t(bits.Get(8));
      continue;
    }
    if (bits.Get(1)) {
      if (!bits.Get(1)) {
        if (!copy(2, bits.Get(8) + 1)) return false;
        continue;
      }
      uint32_t length = 3;
      if (bits.Get(1)) {
        const uint32_t n = bits.Get(8);
        if (!n) break;
        length = n + 8;
      }
      if (!copy(length, distance())) return false;
      continue;
    }
    const uint32_t b = bits.Get(1);
    uint32_t length = 4 + b;
    if (bits.Get(1)) length = ((3 + b) << 1) + bits.Get(1);
    if (length == 9) {
      const uint32_t run = (bits.Get(4) + 3) * 4;
      if (run > decoded - at) return false;
      for (uint32_t i = 0; i < run; ++i) out[at++] = uint8_t(bits.Get(8));
      continue;
    }
    if (!copy(length, distance())) return false;
  }
  return at == decoded && !bits.overrun();
}

void ReadPackagedShaders(const std::filesystem::path& game_data_root,
                         const std::function<void(const uint8_t* data, size_t size)>& add) {
  const std::filesystem::path folder = game_data_root / "Data";
  std::vector<uint8_t> index;
  uint32_t count = 0;
  if (!LoadIndex(folder, "shaders", index, count)) return;
  Archives archives(folder);
  auto read = [&archives](uint32_t part, uint64_t offset, size_t size, std::vector<uint8_t>& out) {
    return archives.Read(part, offset, size, out);
  };

  uint32_t packages = 0, records = 0, entries = 0, failed = 0;
  std::vector<uint8_t> header, block, decoded;
  for (uint32_t i = 0; i < count; ++i) {
    const uint8_t* e = index.data() + kIndexHeader + size_t(i) * kEntrySize;
    const uint32_t type = Be32(e + 8), size = Be32(e + 16), offset = Be32(e + 28), part = Be32(e + 32);
    if (type == kShaderEntryType) {
      if (!read(part, offset, size, block)) {
        ++failed;
        continue;
      }
      ++entries;
      add(block.data(), block.size());
      continue;
    }
    if (type != kGeomType) continue;
    constexpr size_t kPairsEnd = kGeomPairs + kGeomPairCount * 8;
    if (size < kPairsEnd || !read(part, offset, kPairsEnd, header) || std::memcmp(header.data(), "GEOM", 4)) {
      ++failed;
      continue;
    }
    const uint32_t header_size = Be32(header.data() + kGeomHeaderSizeField);
    if (header_size < kPairsEnd || header_size > size || !read(part, offset, header_size, header)) {
      ++failed;
      continue;
    }
    ++packages;
    const size_t pair = kGeomPairs + kShaderPair * 8;
    const uint32_t record_count = Be32(header.data() + pair);
    const uint64_t first_record = pair + 4 + uint64_t(Be32(header.data() + pair + 4));
    if (first_record + uint64_t(record_count) * kRecordSize > header_size) {
      ++failed;
      continue;
    }
    for (uint32_t r = 0; r < record_count; ++r) {
      const uint8_t* record = header.data() + first_record + size_t(r) * kRecordSize;
      const uint32_t at = Be32(record + 16), stored = Be32(record + 20), decoded_size = Be32(record + 24);
      if (Be32(record) >> 24 != kShaderClass || uint64_t(at) + stored > size ||
          !read(part, uint64_t(offset) + at, stored, block)) {
        ++failed;
        continue;
      }
      if (!decoded_size) {  // stored as it is
        ++records;
        add(block.data(), block.size());
        continue;
      }
      if (!DecodeEdl(block.data(), block.size(), decoded) || decoded.size() != decoded_size) {
        ++failed;
        continue;
      }
      ++records;
      add(decoded.data(), decoded.size());
    }
  }
  REXLOG_INFO("[potcawe] shaders: {} records in {} level packages and {} entry, {} unreadable", records, packages,
              entries, failed);
}

std::unordered_map<uint32_t, std::vector<uint8_t>> ReadPackageTextures(const std::filesystem::path& game_data_root,
                                                                       uint32_t package_id,
                                                                       const std::vector<uint32_t>& guids) {
  std::unordered_map<uint32_t, std::vector<uint8_t>> textures;
  const std::filesystem::path folder = game_data_root / "Data";
  std::vector<uint8_t> index;
  uint32_t count = 0;
  if (!LoadIndex(folder, "textures", index, count)) return textures;
  Archives archives(folder);
  for (uint32_t i = 0; i < count; ++i) {
    const uint8_t* e = index.data() + kIndexHeader + size_t(i) * kEntrySize;
    if (Be32(e + 4) != package_id) continue;
    const uint32_t type = Be32(e + 8), size = Be32(e + 16), offset = Be32(e + 28), part = Be32(e + 32);
    constexpr size_t kPairsEnd = kGeomPairs + kGeomPairCount * 8;
    std::vector<uint8_t> header, block, decoded;
    if (type != kGeomType || size < kPairsEnd || !archives.Read(part, offset, kPairsEnd, header) ||
        std::memcmp(header.data(), "GEOM", 4)) {
      break;
    }
    const uint32_t header_size = Be32(header.data() + kGeomHeaderSizeField);
    if (header_size < kPairsEnd || header_size > size || !archives.Read(part, offset, header_size, header)) break;
    const size_t pair = kGeomPairs + kTexturePair * 8;
    const uint32_t record_count = Be32(header.data() + pair);
    const uint64_t first_record = pair + 4 + uint64_t(Be32(header.data() + pair + 4));
    if (first_record + uint64_t(record_count) * kRecordSize > header_size) break;
    for (uint32_t r = 0; r < record_count; ++r) {
      const uint8_t* record = header.data() + first_record + size_t(r) * kRecordSize;
      const uint32_t guid = Be32(record + 4);
      if (std::find(guids.begin(), guids.end(), guid) == guids.end()) continue;
      const uint32_t at = Be32(record + 16), stored = Be32(record + 20), decoded_size = Be32(record + 24);
      if (uint64_t(at) + stored > size || !archives.Read(part, uint64_t(offset) + at, stored, block)) continue;
      if (!decoded_size) {
        textures[guid] = block;
      } else if (DecodeEdl(block.data(), block.size(), decoded) && decoded.size() == decoded_size) {
        textures[guid] = std::move(decoded);
      }
    }
    break;
  }
  return textures;
}

}  // namespace potcawe

// potcawe - startup (see startup.h).

#include "startup.h"

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/logging.h>

#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace potcawe {
namespace {

constexpr const char* kGameExecutable = "default.xex";
// CRC32 of the default.xex this code was recompiled from (6,041,600 bytes, title ID 425607D1,
// media ID 66DCBB9F).
constexpr uint32_t kGameExecutableCrc32 = 0xA31B1069;

constexpr const char kDefaultConfig[] =
    R"(# potcawerecomp.exe runtime configuration, loaded at startup. The game recreates this file with
# these defaults if it is missing. Command line options (--name=value) win over it.
log_level = "info"
)";

uint32_t Crc32(const std::vector<char>& data) {
  static const std::array<uint32_t, 256> table = [] {
    std::array<uint32_t, 256> t{};
    for (uint32_t i = 0; i < 256; ++i) {
      uint32_t c = i;
      for (int k = 0; k < 8; ++k) {
        c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
      }
      t[i] = c;
    }
    return t;
  }();
  uint32_t crc = 0xFFFFFFFFu;
  for (char byte : data) {
    crc = table[(crc ^ uint8_t(byte)) & 0xFF] ^ (crc >> 8);
  }
  return crc ^ 0xFFFFFFFFu;
}

#ifdef _WIN32
// MB_* flags; returns the button pressed.
int ShowMessage(const std::wstring& text, UINT flags) {
  return MessageBoxW(nullptr, text.c_str(), L"Pirates of the Caribbean: At World's End", flags);
}

std::wstring Hex32(uint32_t value) {
  wchar_t buffer[16];
  swprintf(buffer, 16, L"%08X", value);
  return buffer;
}
#endif

}  // namespace

std::filesystem::path DefaultGameRoot() {
  std::filesystem::path next_to_exe = rex::filesystem::GetExecutableFolder() / "assets";
  if (std::filesystem::exists(next_to_exe / kGameExecutable)) {
    return next_to_exe;
  }
  std::filesystem::path repository = POTCAWE_DEFAULT_GAME_ROOT;
  if (std::filesystem::exists(repository / kGameExecutable)) {
    return repository;
  }
  // Neither has the game: report the folder next to the executable (the release layout), not a path
  // of the machine the build was made on.
  return next_to_exe;
}

void ConfigureUserData(std::filesystem::path& user_data_root, std::filesystem::path& cache_root) {
  if (!rex::cvar::Query<std::string>("user_data_root").empty()) {
    return;  // chosen on the command line
  }
  std::filesystem::path portable = rex::filesystem::GetExecutableFolder() / "userdata";
  std::error_code ec;
  std::filesystem::create_directories(portable, ec);
  // Writable? Logging is not set up yet: the folder in use is logged later
  // (PotcaweApp::SetupEnvironment).
  std::filesystem::path probe = portable / ".write_test";
  {
    std::ofstream f(probe, std::ios::binary);
    if (!f) {
      return;
    }
  }
  std::filesystem::remove(probe, ec);
  user_data_root = portable;
  if (rex::cvar::Query<std::string>("cache_root").empty()) {
    cache_root = portable / "cache";
  }
}

void EnsureDefaultConfig(const std::filesystem::path& config_path) {
  if (std::filesystem::exists(config_path)) {
    return;
  }
  std::ofstream f(config_path, std::ios::binary);
  f << kDefaultConfig;
  // Logging is not initialized yet (the config sets it up): nothing to log here.
}

bool CheckGameFiles(const std::filesystem::path& game_root) {
  std::filesystem::path exe = game_root / kGameExecutable;
  std::ifstream f(exe, std::ios::binary);
  if (!f) {
    REXLOG_ERROR("[startup] Game files not found: {} does not exist", exe.string());
#ifdef _WIN32
    ShowMessage(L"Game files not found.\n\n"
                L"Copy the files of your Pirates of the Caribbean: At World's End (Xbox 360) disc "
                L"into:\n" +
                    game_root.wstring() + L"\n\nso that this file exists:\n" + exe.wstring(),
                MB_OK | MB_ICONERROR);
#endif
    return false;
  }
  std::vector<char> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  uint32_t crc = Crc32(data);
  if (crc == kGameExecutableCrc32) {
    REXLOG_INFO("[startup] {}: CRC32 {:08X} (expected version)", exe.string(), crc);
    return true;
  }
  REXLOG_WARN("[startup] {}: CRC32 {:08X}, expected {:08X}: a different version of the game",
              exe.string(), crc, kGameExecutableCrc32);
#ifdef _WIN32
  int answer = ShowMessage(L"default.xex is not the version this build was recompiled from "
                           L"(CRC32 " + Hex32(crc) + L", expected " + Hex32(kGameExecutableCrc32) +
                               L").\nThe game will most likely crash.\n\nStart anyway?",
                           MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
  return answer == IDYES;
#else
  return true;
#endif
}

}  // namespace potcawe

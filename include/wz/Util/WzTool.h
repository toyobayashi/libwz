#ifndef WZ_UTIL_WZTOOL_H_
#define WZ_UTIL_WZTOOL_H_
#include <array>
#include <cstdint>

#ifdef _WIN32
#include <filesystem>
#endif

#include <string>
#include "wz/WzEnums.h"

namespace wz {

class WzTool {
 public:
  static constexpr int32_t WZ_HEADER = 0x31474B50;

#ifdef _WIN32
  static std::filesystem::path ToPath(const std::string& utf8_path);
#endif
  static uint32_t RotateLeft(uint32_t x, uint8_t n);
  static uint32_t RotateRight(uint32_t x, uint8_t n);
  static int GetCompressedIntLength(int i);
  static int GetEncodedStringLength(const std::string& s);
  static int GetWzObjectValueLength(const std::string& s, uint8_t type);
  static void ClearWzObjectValueLengthCache();
  static std::array<uint8_t, 4> GetIvByMapleVersion(WzMapleVersion ver);
  static WzMapleVersion DetectMapleVersion(const std::string& wzFilePath,
                                           int16_t* fileVersion);
  static bool IsListFile(const std::string& path);
  static bool IsDataWzHotfixFile(const std::string& path);

 private:
  static int GetRecognizedCharacters(const std::string& source);
  static double GetDecryptionSuccessRate(const std::string& wzPath,
                                         WzMapleVersion encVersion,
                                         int16_t* version);
};

}  // namespace wz
#endif  // WZ_UTIL_WZTOOL_H_

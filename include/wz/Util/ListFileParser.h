#ifndef WZ_UTIL_LISTFILEPARSER_H_
#define WZ_UTIL_LISTFILEPARSER_H_
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "wz/WzEnums.h"

namespace wz {

class ListFileParser {
 public:
  static std::vector<std::string> ParseListFile(const std::string& filePath,
                                                WzMapleVersion version);
  static std::vector<std::string> ParseListFile(
      const std::string& filePath, const std::array<uint8_t, 4>& WzIv);
};

}  // namespace wz
#endif  // WZ_UTIL_LISTFILEPARSER_H_

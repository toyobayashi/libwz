#include "wz/Util/ListFileParser.h"
#include <limits>
#include <memory>
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzPath.h"
#include "wz/Util/WzStream.h"
#include "wz/Util/WzTool.h"

namespace wz {

std::vector<std::string> ListFileParser::ParseListFile(
    const std::string& filePath, WzMapleVersion version) {
  return ParseListFile(filePath, WzTool::GetIvByMapleVersion(version));
}

std::vector<std::string> ListFileParser::ParseListFile(
    const std::string& filePath, const std::array<uint8_t, 4>& WzIv) {
  std::vector<std::string> listEntries;

  WzFileStream fs;
  if (!fs.Open(wz::to_path(filePath), "rb")) return listEntries;

  WzBinaryReader reader(std::make_shared<WzFileDataSource>(&fs), WzIv);

  while (reader.Available() > 0) {
    int32_t len = reader.ReadInt32();
    if (len < 0 || len > 100000) break;

    std::u16string chars(static_cast<size_t>(len), u'\0');
    for (int32_t i = 0; i < len; i++) {
      chars[static_cast<size_t>(i)] = static_cast<char16_t>(reader.ReadInt16());
    }
    reader.ReadUInt16();

    listEntries.push_back(reader.DecryptString(chars));
  }

  if (!listEntries.empty()) {
    auto& lastEntry = listEntries.back();
    if (!lastEntry.empty()) {
      lastEntry.back() = 'g';
    }
  }

  return listEntries;
}

}  // namespace wz

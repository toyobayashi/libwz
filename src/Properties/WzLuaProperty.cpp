#include "wz/Properties/WzLuaProperty.h"
#include <filesystem>
#include <fstream>
#include "wz/Util/WzKeyGenerator.h"
#include "wz/Util/WzPath.h"

namespace wz {
WzLuaProperty::WzLuaProperty(const std::string& name,
                             const std::vector<uint8_t>& encryptedBytes)
    : encryptedBytes_(encryptedBytes),
      wzKey_(WzKeyGenerator::GenerateLuaWzKey()) {
  SetName(name);
}

std::string WzLuaProperty::GetString() const {
  auto decoded = EncodeDecode(encryptedBytes_);
  return std::string(decoded.begin(), decoded.end());
}

std::vector<uint8_t> WzLuaProperty::EncodeDecode(
    const std::vector<uint8_t>& input) const {
  std::vector<uint8_t> result(input.size());
  for (size_t i = 0; i < input.size(); i++) {
    result[i] = input[i] ^ wzKey_[i];
  }
  return result;
}

bool WzLuaProperty::SaveToFile(const std::string& filePath) {
  std::string str = GetString();
  std::error_code ec;
  std::filesystem::create_directories(wz::to_path(filePath).parent_path(), ec);
  if (ec) return false;
  std::ofstream out(wz::to_path(filePath));
  if (!out) return false;
  out << str;
  return true;
}

}  // namespace wz

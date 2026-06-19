#include "wz/Properties/WzLuaProperty.h"
#include <filesystem>
#include <fstream>
#include "wz/Util/WzBinaryWriter.h"
#include "wz/Util/WzKeyGenerator.h"
#include "wz/Util/WzPath.h"

namespace wz {
WzLuaProperty::WzLuaProperty(const std::string& name,
                             const std::vector<uint8_t>& encryptedBytes)
    : encryptedBytes_(encryptedBytes),
      wzKey_(WzKeyGenerator::GenerateLuaWzKey()) {
  SetName(name);
}

Result<void> WzLuaProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(0x01);
  writer->WriteCompressedInt(static_cast<int32_t>(encryptedBytes_.size()));
  writer->BaseStream().write(
      reinterpret_cast<const char*>(encryptedBytes_.data()),
      static_cast<std::streamsize>(encryptedBytes_.size()));
  return {};
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

Result<void> WzLuaProperty::SaveToFile(const std::string& filePath) {
  std::string str = GetString();
  auto outPath = wz::to_path(filePath);
  auto parentPath = outPath.parent_path();
  std::error_code ec;
  if (!parentPath.empty()) {
    std::filesystem::create_directories(parentPath, ec);
    if (ec) return std::unexpected(Error::IoError(ec.message()));
  }
  std::ofstream out(outPath);
  if (!out)
    return std::unexpected(Error::IoError("Failed to open file for writing"));
  out << str;
  return {};
}

}  // namespace wz

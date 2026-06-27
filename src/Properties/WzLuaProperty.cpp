#include "wz/Properties/WzLuaProperty.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/Util/WzKeyGenerator.h"

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
  if (!writer->BaseStream().Write(encryptedBytes_.data(),
                                  encryptedBytes_.size())) {
    return std::unexpected(Error::IoError("Failed to write Lua property"));
  }
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

}  // namespace wz

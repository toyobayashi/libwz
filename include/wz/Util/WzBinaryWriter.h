#ifndef WZ_UTIL_WZBINARYWRITER_H_
#define WZ_UTIL_WZBINARYWRITER_H_
#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <unordered_map>
#include "wz/Util/Defines.h"
#include "wz/Util/WzKeyGenerator.h"
#include "wz/Util/WzMutableKey.h"
#include "wz/WzEnums.h"
#include "wz/WzHeader.h"

namespace wz {

class WzBinaryWriter {
 public:
  WzBinaryWriter(std::ostream& output, const std::array<uint8_t, 4>& WzIv);
  WzBinaryWriter(std::ostream& output,
                 const std::array<uint8_t, 4>& WzIv,
                 uint32_t hash);
  ~WzBinaryWriter() = default;
  WZ_DISALLOW_COPY(WzBinaryWriter)
  WzBinaryWriter(WzBinaryWriter&& other) noexcept;
  WzBinaryWriter& operator=(WzBinaryWriter&& other) noexcept;

  WzMutableKey& GetWzKey() { return wzKey_; }
  uint32_t Hash() const { return hash_; }
  void SetHash(uint32_t h) { hash_ = h; }
  WzHeader* GetHeader() { return &header_; }
  void SetHeader(const WzHeader& h) { header_ = h; }
  std::ostream& BaseStream() { return *output_; }
  void ClearStringCache();

  int64_t Position();

  void WriteByte(uint8_t value);
  void WriteSByte(int8_t value);
  void WriteInt16(int16_t value);
  void WriteInt32(int32_t value);
  void WriteInt64(int64_t value);
  void WriteUInt16(uint16_t value);
  void WriteUInt32(uint32_t value);
  void WriteUInt64(uint64_t value);
  void WriteSingle(float value);
  void WriteDouble(double value);
  void WriteString(const std::string& value);
  void WriteNullTerminatedString(const std::string& value);
  void WriteStringValue(const std::string& value,
                        uint8_t withoutOffset,
                        uint8_t withOffset);
  bool WriteWzObjectValue(const std::string& value, WzDirectoryType type);
  void WriteCompressedInt(int32_t value);
  void WriteCompressedLong(int64_t value);
  void WriteOffset(int64_t value);

 private:
  std::ostream* output_;
  WzMutableKey wzKey_;
  uint32_t hash_ = 0;
  WzHeader header_ = WzHeader::GetDefault();
  std::unordered_map<std::string, int32_t> stringCache_;

  template <typename T>
  void WriteLittleEndian(T value);
  void WriteAsciiString(const std::string& value);
  void WriteUnicodeString(const std::u16string& value);
};

}  // namespace wz
#endif  // WZ_UTIL_WZBINARYWRITER_H_

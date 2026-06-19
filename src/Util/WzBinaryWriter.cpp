#include "wz/Util/WzBinaryWriter.h"
#include <cstring>
#include <utility>
#include <vector>
#include "wz/Util/WzTool.h"
#include "wz/WzAESConstant.h"

namespace wz {

namespace {

std::u16string Utf8ToUtf16(const std::string& value) {
  std::u16string result;
  result.reserve(value.size());

  for (size_t i = 0; i < value.size();) {
    uint8_t ch = static_cast<uint8_t>(value[i]);
    uint32_t codePoint = 0;
    size_t extraBytes = 0;

    if ((ch & 0x80) == 0) {
      codePoint = ch;
    } else if ((ch & 0xE0) == 0xC0) {
      codePoint = ch & 0x1F;
      extraBytes = 1;
    } else if ((ch & 0xF0) == 0xE0) {
      codePoint = ch & 0x0F;
      extraBytes = 2;
    } else if ((ch & 0xF8) == 0xF0) {
      codePoint = ch & 0x07;
      extraBytes = 3;
    } else {
      result.push_back(static_cast<char16_t>(ch));
      ++i;
      continue;
    }

    if (i + extraBytes >= value.size()) {
      result.push_back(static_cast<char16_t>(ch));
      ++i;
      continue;
    }

    bool valid = true;
    for (size_t j = 1; j <= extraBytes; ++j) {
      uint8_t next = static_cast<uint8_t>(value[i + j]);
      if ((next & 0xC0) != 0x80) {
        valid = false;
        break;
      }
      codePoint = (codePoint << 6) | (next & 0x3F);
    }

    if (!valid) {
      result.push_back(static_cast<char16_t>(ch));
      ++i;
      continue;
    }

    if (codePoint <= 0xFFFF) {
      result.push_back(static_cast<char16_t>(codePoint));
    } else {
      codePoint -= 0x10000;
      result.push_back(static_cast<char16_t>(0xD800 + (codePoint >> 10)));
      result.push_back(static_cast<char16_t>(0xDC00 + (codePoint & 0x3FF)));
    }
    i += extraBytes + 1;
  }

  return result;
}

bool NeedsUnicode(const std::u16string& value) {
  for (char16_t ch : value) {
    if (ch > INT8_MAX) return true;
  }
  return false;
}

}  // namespace

WzBinaryWriter::WzBinaryWriter(std::ostream& output,
                               const std::array<uint8_t, 4>& WzIv)
    : output_(&output), wzKey_(WzKeyGenerator::GenerateWzKey(WzIv)) {}

WzBinaryWriter::WzBinaryWriter(std::ostream& output,
                               const std::array<uint8_t, 4>& WzIv,
                               uint32_t hash)
    : output_(&output),
      wzKey_(WzKeyGenerator::GenerateWzKey(WzIv)),
      hash_(hash) {}

WzBinaryWriter::WzBinaryWriter(WzBinaryWriter&& other) noexcept
    : output_(other.output_),
      wzKey_(std::move(other.wzKey_)),
      hash_(other.hash_),
      header_(other.header_),
      stringCache_(std::move(other.stringCache_)) {
  other.output_ = nullptr;
}

WzBinaryWriter& WzBinaryWriter::operator=(WzBinaryWriter&& other) noexcept {
  if (this != &other) {
    output_ = other.output_;
    wzKey_ = std::move(other.wzKey_);
    hash_ = other.hash_;
    header_ = other.header_;
    stringCache_ = std::move(other.stringCache_);
    other.output_ = nullptr;
  }
  return *this;
}

int64_t WzBinaryWriter::Position() {
  return static_cast<int64_t>(output_->tellp());
}

template <typename T>
void WzBinaryWriter::WriteLittleEndian(T value) {
  uint8_t bytes[sizeof(T)];
  std::memcpy(bytes, &value, sizeof(T));
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  for (size_t i = 0; i < sizeof(T) / 2; ++i) {
    std::swap(bytes[i], bytes[sizeof(T) - 1 - i]);
  }
#endif
  output_->write(reinterpret_cast<const char*>(bytes),
                 static_cast<std::streamsize>(sizeof(T)));
}

void WzBinaryWriter::WriteByte(uint8_t value) {
  output_->put(static_cast<char>(value));
}

void WzBinaryWriter::WriteSByte(int8_t value) {
  WriteByte(static_cast<uint8_t>(value));
}

void WzBinaryWriter::WriteInt16(int16_t value) {
  WriteLittleEndian(value);
}

void WzBinaryWriter::WriteInt32(int32_t value) {
  WriteLittleEndian(value);
}

void WzBinaryWriter::WriteInt64(int64_t value) {
  WriteLittleEndian(value);
}

void WzBinaryWriter::WriteUInt16(uint16_t value) {
  WriteLittleEndian(value);
}

void WzBinaryWriter::WriteUInt32(uint32_t value) {
  WriteLittleEndian(value);
}

void WzBinaryWriter::WriteUInt64(uint64_t value) {
  WriteLittleEndian(value);
}

void WzBinaryWriter::WriteSingle(float value) {
  WriteLittleEndian(value);
}

void WzBinaryWriter::WriteDouble(double value) {
  WriteLittleEndian(value);
}

void WzBinaryWriter::WriteString(const std::string& value) {
  if (value.empty()) {
    WriteByte(0);
    return;
  }

  std::u16string utf16 = Utf8ToUtf16(value);
  if (NeedsUnicode(utf16)) {
    WriteUnicodeString(utf16);
  } else {
    WriteAsciiString(value);
  }
}

void WzBinaryWriter::WriteAsciiString(const std::string& value) {
  const int32_t length = static_cast<int32_t>(value.length());
  if (length > INT8_MAX) {
    WriteSByte(INT8_MIN);
    WriteInt32(length);
  } else {
    WriteSByte(static_cast<int8_t>(-length));
  }

  uint8_t mask = 0xAA;
  for (int32_t i = 0; i < length; ++i) {
    uint8_t encryptedChar = static_cast<uint8_t>(value[i]);
    encryptedChar ^= wzKey_[i];
    encryptedChar ^= mask;
    WriteByte(encryptedChar);
    ++mask;
  }
}

void WzBinaryWriter::WriteUnicodeString(const std::u16string& value) {
  const int32_t length = static_cast<int32_t>(value.length());
  if (length >= INT8_MAX) {
    WriteSByte(INT8_MAX);
    WriteInt32(length);
  } else {
    WriteSByte(static_cast<int8_t>(length));
  }

  uint16_t mask = 0xAAAA;
  for (int32_t i = 0; i < length; ++i) {
    uint16_t encryptedChar = static_cast<uint16_t>(value[i]);
    encryptedChar ^=
        static_cast<uint16_t>((wzKey_[i * 2 + 1] << 8) + wzKey_[i * 2]);
    encryptedChar ^= mask;
    WriteUInt16(encryptedChar);
    ++mask;
  }
}

void WzBinaryWriter::WriteNullTerminatedString(const std::string& value) {
  output_->write(value.data(), static_cast<std::streamsize>(value.size()));
  WriteByte(0);
}

void WzBinaryWriter::WriteStringValue(const std::string& value,
                                      uint8_t withoutOffset,
                                      uint8_t withOffset) {
  auto found = stringCache_.find(value);
  if (value.length() > 4 && found != stringCache_.end()) {
    WriteByte(withOffset);
    WriteInt32(found->second);
    return;
  }

  WriteByte(withoutOffset);
  int32_t stringOffset = static_cast<int32_t>(Position());
  WriteString(value);
  stringCache_.try_emplace(value, stringOffset);
}

bool WzBinaryWriter::WriteWzObjectValue(const std::string& value,
                                        WzDirectoryType type) {
  std::string storeName =
      std::to_string(static_cast<uint8_t>(type)) + "_" + value;
  auto found = stringCache_.find(storeName);
  if (value.length() > 4 && found != stringCache_.end()) {
    WriteByte(
        static_cast<uint8_t>(WzDirectoryType::RetrieveStringFromOffset_2));
    WriteInt32(found->second);
    return true;
  }

  int32_t stringOffset =
      static_cast<int32_t>(Position() - static_cast<int64_t>(header_.FStart()));
  WriteByte(static_cast<uint8_t>(type));
  WriteString(value);
  stringCache_.try_emplace(storeName, stringOffset);
  return false;
}

void WzBinaryWriter::WriteCompressedInt(int32_t value) {
  if (value > INT8_MAX || value <= INT8_MIN) {
    WriteSByte(INT8_MIN);
    WriteInt32(value);
  } else {
    WriteSByte(static_cast<int8_t>(value));
  }
}

void WzBinaryWriter::WriteCompressedLong(int64_t value) {
  if (value > INT8_MAX || value <= INT8_MIN) {
    WriteSByte(INT8_MIN);
    WriteInt64(value);
  } else {
    WriteSByte(static_cast<int8_t>(value));
  }
}

void WzBinaryWriter::WriteOffset(int64_t value) {
  uint32_t encOffset = static_cast<uint32_t>(Position());
  encOffset = (encOffset - header_.FStart()) ^ 0xFFFFFFFFu;
  encOffset *= hash_;
  encOffset -= WzAESConstant::WZ_OffsetConstant;
  encOffset =
      WzTool::RotateLeft(encOffset, static_cast<uint8_t>(encOffset & 0x1F));
  uint32_t writeOffset =
      encOffset ^ (static_cast<uint32_t>(value) - (header_.FStart() * 2));
  WriteUInt32(writeOffset);
}

}  // namespace wz

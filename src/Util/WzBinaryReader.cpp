#include "wz/Util/WzBinaryReader.h"
#include <cstring>
#include <utility>
#include "wz/Util/WzTool.h"
#include "wz/WzAESConstant.h"
#include "wz/WzImage.h"

namespace wz {

// Simple UTF-16 to UTF-8 conversion (no deprecated codecvt)
static std::string Utf16ToUtf8(const std::u16string& utf16) {
  std::string result;
  result.reserve(utf16.size() * 3);
  for (char16_t ch : utf16) {
    if (ch < 0x80) {
      result.push_back(static_cast<char>(ch));
    } else if (ch < 0x800) {
      result.push_back(static_cast<char>(0xC0 | (ch >> 6)));
      result.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
    } else {
      result.push_back(static_cast<char>(0xE0 | (ch >> 12)));
      result.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
      result.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
    }
  }
  return result;
}

WzBinaryReader::WzBinaryReader(std::ifstream* input,
                               const std::array<uint8_t, 4>& WzIv,
                               int64_t startOffset)
    : input_(input),
      wzKey_(WzKeyGenerator::GenerateWzKey(WzIv)),
      startOffset_(startOffset) {}

WzBinaryReader::~WzBinaryReader() {
  Close();
}

WzBinaryReader::WzBinaryReader(WzBinaryReader&& other) noexcept
    : input_(other.input_),
      wzKey_(std::move(other.wzKey_)),
      hash_(other.hash_),
      header_(other.header_),
      startOffset_(other.startOffset_) {
  other.input_ = nullptr;
  other.header_ = nullptr;
}

WzBinaryReader& WzBinaryReader::operator=(WzBinaryReader&& other) noexcept {
  if (this != &other) {
    Close();
    input_ = other.input_;
    wzKey_ = std::move(other.wzKey_);
    hash_ = other.hash_;
    header_ = other.header_;
    startOffset_ = other.startOffset_;
    other.input_ = nullptr;
    other.header_ = nullptr;
  }
  return *this;
}

uint8_t WzBinaryReader::ReadByte() {
  uint8_t b;
  input_->read(reinterpret_cast<char*>(&b), 1);
  return b;
}

int8_t WzBinaryReader::ReadSByte() {
  int8_t b;
  input_->read(reinterpret_cast<char*>(&b), 1);
  return b;
}

int16_t WzBinaryReader::ReadInt16() {
  int16_t v;
  input_->read(reinterpret_cast<char*>(&v), 2);
  return v;
}

int32_t WzBinaryReader::ReadInt32() {
  int32_t v;
  input_->read(reinterpret_cast<char*>(&v), 4);
  return v;
}

int64_t WzBinaryReader::ReadInt64() {
  int64_t v;
  input_->read(reinterpret_cast<char*>(&v), 8);
  return v;
}

uint16_t WzBinaryReader::ReadUInt16() {
  uint16_t v;
  input_->read(reinterpret_cast<char*>(&v), 2);
  return v;
}

uint32_t WzBinaryReader::ReadUInt32() {
  uint32_t v;
  input_->read(reinterpret_cast<char*>(&v), 4);
  return v;
}

uint64_t WzBinaryReader::ReadUInt64() {
  uint64_t v;
  input_->read(reinterpret_cast<char*>(&v), 8);
  return v;
}

float WzBinaryReader::ReadSingle() {
  float v;
  input_->read(reinterpret_cast<char*>(&v), 4);
  return v;
}

double WzBinaryReader::ReadDouble() {
  double v;
  input_->read(reinterpret_cast<char*>(&v), 8);
  return v;
}

std::vector<uint8_t> WzBinaryReader::ReadBytes(size_t count) {
  std::vector<uint8_t> buf(count);
  input_->read(reinterpret_cast<char*>(buf.data()), count);
  return buf;
}

int64_t WzBinaryReader::Position() {
  return static_cast<int64_t>(input_->tellg());
}

void WzBinaryReader::SetPosition(int64_t pos) {
  input_->seekg(pos);
}

void WzBinaryReader::Seek(int64_t offset, int origin) {
  input_->seekg(offset, static_cast<std::ios_base::seekdir>(origin));
}

std::string WzBinaryReader::DecodeAscii(int length) {
  std::string result(length, '\0');
  uint8_t mask = 0xAA;
  for (int i = 0; i < length; i++) {
    uint8_t encryptedChar = ReadByte();
    encryptedChar ^= mask;
    encryptedChar ^= wzKey_[i];
    result[i] = static_cast<char>(encryptedChar);
    mask++;
  }
  return result;
}

std::string WzBinaryReader::DecodeUnicode(int length) {
  std::u16string utf16(length, u'\0');
  uint16_t mask = 0xAAAA;
  for (int i = 0; i < length; i++) {
    uint16_t encryptedChar = ReadUInt16();
    encryptedChar ^= mask;
    encryptedChar ^=
        static_cast<uint16_t>((wzKey_[i * 2 + 1] << 8) + wzKey_[i * 2]);
    utf16[i] = static_cast<char16_t>(encryptedChar);
    mask++;
  }
  return Utf16ToUtf8(utf16);
}

std::string WzBinaryReader::ReadString() {
  int8_t smallLength = ReadSByte();
  if (smallLength == 0) return {};

  int length;
  if (smallLength > 0)
    length =
        (smallLength == INT8_MAX) ? ReadInt32() : static_cast<int>(smallLength);
  else
    length = (smallLength == INT8_MIN) ? ReadInt32()
                                       : static_cast<int>(-smallLength);

  if (length <= 0) return {};

  if (smallLength > 0)
    return DecodeUnicode(length);
  else
    return DecodeAscii(length);
}

std::string WzBinaryReader::ReadString(size_t length) {
  std::vector<char> buf(length);
  input_->read(buf.data(), length);
  return std::string(buf.data(), length);
}

std::string WzBinaryReader::ReadNullTerminatedString() {
  std::string result;
  char c;
  while (input_->read(&c, 1) && c != '\0') {
    result += c;
  }
  return result;
}

int32_t WzBinaryReader::ReadCompressedInt() {
  int8_t sb = ReadSByte();
  if (sb == INT8_MIN) {
    return ReadInt32();
  }
  return sb;
}

int64_t WzBinaryReader::ReadLong() {
  int8_t sb = ReadSByte();
  if (sb == INT8_MIN) {
    return ReadInt64();
  }
  return sb;
}

int64_t WzBinaryReader::Available() {
  auto cur = input_->tellg();
  input_->seekg(0, std::ios::end);
  auto end = input_->tellg();
  input_->seekg(cur);
  return static_cast<int64_t>(end - cur);
}

int64_t WzBinaryReader::ReadOffset() {
  uint32_t offset = static_cast<uint32_t>(Position());
  offset = (offset - header_->FStart()) ^ 0xFFFFFFFF;
  offset *= hash_;
  offset -= WzAESConstant::WZ_OffsetConstant;
  offset = WzTool::RotateLeft(offset, static_cast<uint8_t>(offset & 0x1F));
  uint32_t encryptedOffset = ReadUInt32();
  offset ^= encryptedOffset;
  offset += header_->FStart() * 2;
  return static_cast<int64_t>(offset) + startOffset_;
}

std::string WzBinaryReader::ReadStringAtOffset(int64_t offset, bool readByte) {
  int64_t currentOffset = Position();
  SetPosition(offset - startOffset_);
  if (readByte) {
    ReadByte();
  }
  std::string returnString = ReadString();
  SetPosition(currentOffset);
  return returnString;
}

std::string WzBinaryReader::ReadStringBlock(int64_t offset) {
  uint8_t flag = ReadByte();
  switch (flag) {
    case 0:
    case WzImage::WzImageHeaderByte_WithoutOffset:
      return ReadString();
    case 1:
    case WzImage::WzImageHeaderByte_WithOffset:
      return ReadStringAtOffset(offset + ReadInt32());
    default:
      return {};
  }
}

void WzBinaryReader::SetOffsetFromFStartToPosition(int offset) {
  SetPosition(static_cast<int64_t>(header_->FStart() + offset) - startOffset_);
}

Result<void> WzBinaryReader::RollbackStreamPosition(int byOffset) {
  int64_t pos = Position() - static_cast<int64_t>(byOffset);
  if (pos < startOffset_)
    return std::unexpected(
        Error::IoError("Cant rollback stream position below 0"));
  SetPosition(pos);
  return {};
}

std::string WzBinaryReader::DecryptString(
    const std::u16string& stringToDecrypt) {
  size_t len = stringToDecrypt.length();
  std::u16string output(len, u'\0');
  for (size_t i = 0; i < len; i++) {
    output[i] = static_cast<char16_t>(
        static_cast<uint16_t>(stringToDecrypt[i]) ^
        static_cast<uint16_t>((wzKey_[i * 2 + 1] << 8) + wzKey_[i * 2]));
  }
  return Utf16ToUtf8(output);
}

std::string WzBinaryReader::DecryptNonUnicodeString(
    const std::u16string& stringToDecrypt) {
  size_t len = stringToDecrypt.length();
  std::u16string output(len, u'\0');
  for (size_t i = 0; i < len; i++) {
    output[i] =
        static_cast<char16_t>(static_cast<uint16_t>(stringToDecrypt[i]) ^
                              static_cast<uint16_t>(wzKey_[i]));
  }
  return Utf16ToUtf8(output);
}

void WzBinaryReader::PrintHexBytes(int numberOfBytes) {
  // Debug only
  (void)numberOfBytes;
}

void WzBinaryReader::Close() {
  if (input_ && input_->is_open()) {
    input_->close();
  }
}

}  // namespace wz

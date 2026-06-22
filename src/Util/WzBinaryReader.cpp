#include "wz/Util/WzBinaryReader.h"
#include <algorithm>
#include <cstring>
#include <limits>
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

WzBinaryReader::WzBinaryReader(std::istream& input,
                               const std::array<uint8_t, 4>& WzIv,
                               int64_t startOffset)
    : WzBinaryReader(
          std::make_shared<WzStreamDataSource>(input), WzIv, startOffset) {}

WzBinaryReader::WzBinaryReader(std::shared_ptr<WzDataSource> source,
                               const std::array<uint8_t, 4>& WzIv,
                               int64_t startOffset)
    : source_(std::move(source)),
      position_(startOffset),
      cache_(kCacheSize),
      wzKey_(WzKeyGenerator::GenerateWzKey(WzIv)),
      startOffset_(startOffset) {}

WzBinaryReader::~WzBinaryReader() {
  Close();
}

WzBinaryReader::WzBinaryReader(WzBinaryReader&& other) noexcept
    : source_(std::move(other.source_)),
      position_(other.position_),
      cache_(std::move(other.cache_)),
      cacheOffset_(other.cacheOffset_),
      cacheSize_(other.cacheSize_),
      wzKey_(std::move(other.wzKey_)),
      hash_(other.hash_),
      header_(other.header_),
      startOffset_(other.startOffset_) {
  other.header_ = nullptr;
  other.position_ = 0;
  other.cacheOffset_ = 0;
  other.cacheSize_ = 0;
}

WzBinaryReader& WzBinaryReader::operator=(WzBinaryReader&& other) noexcept {
  if (this != &other) {
    Close();
    source_ = std::move(other.source_);
    position_ = other.position_;
    cache_ = std::move(other.cache_);
    cacheOffset_ = other.cacheOffset_;
    cacheSize_ = other.cacheSize_;
    wzKey_ = std::move(other.wzKey_);
    hash_ = other.hash_;
    header_ = other.header_;
    startOffset_ = other.startOffset_;
    other.header_ = nullptr;
    other.position_ = 0;
    other.cacheOffset_ = 0;
    other.cacheSize_ = 0;
  }
  return *this;
}

uint8_t WzBinaryReader::ReadByte() {
  uint8_t b = 0;
  ReadExact(&b, sizeof(b));
  return b;
}

int8_t WzBinaryReader::ReadSByte() {
  int8_t b = 0;
  ReadExact(&b, sizeof(b));
  return b;
}

int16_t WzBinaryReader::ReadInt16() {
  int16_t v = 0;
  ReadExact(&v, sizeof(v));
  return v;
}

int32_t WzBinaryReader::ReadInt32() {
  int32_t v = 0;
  ReadExact(&v, sizeof(v));
  return v;
}

int64_t WzBinaryReader::ReadInt64() {
  int64_t v = 0;
  ReadExact(&v, sizeof(v));
  return v;
}

uint16_t WzBinaryReader::ReadUInt16() {
  uint16_t v = 0;
  ReadExact(&v, sizeof(v));
  return v;
}

uint32_t WzBinaryReader::ReadUInt32() {
  uint32_t v = 0;
  ReadExact(&v, sizeof(v));
  return v;
}

uint64_t WzBinaryReader::ReadUInt64() {
  uint64_t v = 0;
  ReadExact(&v, sizeof(v));
  return v;
}

float WzBinaryReader::ReadSingle() {
  float v = 0;
  ReadExact(&v, sizeof(v));
  return v;
}

double WzBinaryReader::ReadDouble() {
  double v = 0;
  ReadExact(&v, sizeof(v));
  return v;
}

std::vector<uint8_t> WzBinaryReader::ReadBytes(size_t count) {
  std::vector<uint8_t> buf(count);
  if (!ReadExact(buf.data(), count)) return {};
  return buf;
}

int64_t WzBinaryReader::Position() {
  return position_;
}

void WzBinaryReader::SetPosition(int64_t pos) {
  position_ = pos;
}

void WzBinaryReader::Seek(int64_t offset, int origin) {
  int64_t base = 0;
  if (origin == static_cast<int>(std::ios_base::cur)) {
    base = position_;
  } else if (origin == static_cast<int>(std::ios_base::end)) {
    const uint64_t size = SourceSize();
    base = size > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())
               ? std::numeric_limits<int64_t>::max()
               : static_cast<int64_t>(size);
  }
  position_ = base + offset;
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
  ReadExact(buf.data(), length);
  return std::string(buf.data(), length);
}

std::string WzBinaryReader::ReadNullTerminatedString() {
  std::string result;
  char c = '\0';
  while (ReadExact(&c, 1) && c != '\0') {
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
  if (!source_ || position_ < 0) return 0;
  const uint64_t pos = static_cast<uint64_t>(position_);
  const uint64_t size = source_->Size();
  if (pos >= size) return 0;
  const uint64_t available = size - pos;
  if (available > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
    return std::numeric_limits<int64_t>::max();
  }
  return static_cast<int64_t>(available);
}

uint64_t WzBinaryReader::SourceSize() const {
  return source_ ? source_->Size() : 0;
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
  source_.reset();
  cache_.clear();
  cacheOffset_ = 0;
  cacheSize_ = 0;
}

bool WzBinaryReader::ReadExact(void* destination, size_t count) {
  if (count == 0) return true;
  auto* output = static_cast<uint8_t*>(destination);
  size_t total = 0;

  while (total < count) {
    if (position_ < 0) return false;
    const uint64_t current = static_cast<uint64_t>(position_);
    const uint64_t cacheEnd = cacheOffset_ + cacheSize_;
    if (cacheSize_ == 0 || current < cacheOffset_ || current >= cacheEnd) {
      if (!FillCache()) return false;
      if (cacheSize_ == 0) return false;
    }

    const size_t cacheIndex = static_cast<size_t>(current - cacheOffset_);
    const size_t available = cacheSize_ - cacheIndex;
    const size_t toCopy = std::min(count - total, available);
    std::memcpy(output + total, cache_.data() + cacheIndex, toCopy);
    total += toCopy;
    position_ += static_cast<int64_t>(toCopy);
  }
  return true;
}

bool WzBinaryReader::FillCache() {
  if (!source_ || position_ < 0) return false;
  cacheOffset_ = static_cast<uint64_t>(position_);
  cacheSize_ = 0;
  auto readResult = source_->ReadAt(cacheOffset_, cache_);
  if (!readResult.has_value()) return false;
  cacheSize_ = readResult.value();
  return true;
}

}  // namespace wz

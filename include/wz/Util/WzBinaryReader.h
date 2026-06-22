#ifndef WZ_UTIL_WZBINARYREADER_H_
#define WZ_UTIL_WZBINARYREADER_H_
#include <cstdint>
#include <istream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "wz/Result.h"
#include "wz/Util/Defines.h"
#include "wz/Util/WzDataSource.h"
#include "wz/Util/WzKeyGenerator.h"
#include "wz/Util/WzMutableKey.h"
#include "wz/WzEnums.h"
#include "wz/WzHeader.h"

namespace wz {

class WzBinaryReader {
 public:
  WzBinaryReader(std::istream& input,
                 const std::array<uint8_t, 4>& wzIv,
                 int64_t startOffset = 0);
  WzBinaryReader(std::shared_ptr<WzDataSource> source,
                 const std::array<uint8_t, 4>& wzIv,
                 int64_t startOffset = 0);
  ~WzBinaryReader();
  WZ_DISALLOW_COPY(WzBinaryReader)
  WzBinaryReader(WzBinaryReader&& other) noexcept;
  WzBinaryReader& operator=(WzBinaryReader&& other) noexcept;

  WzMutableKey& GetWzKey() { return wzKey_; }
  std::recursive_mutex& Mutex() const { return mutex_; }
  uint32_t Hash() const { return hash_; }
  void SetHash(uint32_t h) { hash_ = h; }
  WzHeader* GetHeader() { return header_; }
  void SetHeader(WzHeader* h) { header_ = h; }
  int64_t StartOffset() const { return startOffset_; }

  void SetOffsetFromFStartToPosition(int offset);
  Result<void> RollbackStreamPosition(int byOffset);

  std::string ReadString();
  std::string ReadString(size_t length);
  std::string ReadNullTerminatedString();
  int32_t ReadCompressedInt();
  int64_t ReadLong();
  int64_t Available();
  uint64_t SourceSize() const;
  int64_t ReadOffset();

  std::string ReadStringAtOffset(int64_t offset, bool readByte = false);
  std::string DecryptString(const std::u16string& stringToDecrypt);
  std::string DecryptNonUnicodeString(const std::u16string& stringToDecrypt);
  std::string ReadStringBlock(int64_t offset);

  // Base stream wrappers
  uint8_t ReadByte();
  int16_t ReadInt16();
  int32_t ReadInt32();
  int64_t ReadInt64();
  uint16_t ReadUInt16();
  uint32_t ReadUInt32();
  uint64_t ReadUInt64();
  float ReadSingle();
  double ReadDouble();
  std::vector<uint8_t> ReadBytes(size_t count);
  int8_t ReadSByte();

  int64_t Position();
  void SetPosition(int64_t pos);
  void Seek(int64_t offset, int origin);
  void PrintHexBytes(int numberOfBytes);
  void Close();

  // Note: WzKey must be accessed via GetWzKey() which returns non-const
  // reference

 private:
  static constexpr size_t kCacheSize = 256 * 1024;

  std::shared_ptr<WzDataSource> source_;
  int64_t position_ = 0;
  std::vector<uint8_t> cache_;
  uint64_t cacheOffset_ = 0;
  size_t cacheSize_ = 0;
  WzMutableKey wzKey_;
  uint32_t hash_ = 0;
  WzHeader* header_ = nullptr;
  int64_t startOffset_ = 0;
  mutable std::recursive_mutex mutex_;

  std::string DecodeUnicode(int length);
  std::string DecodeAscii(int length);
  bool ReadExact(void* destination, size_t count);
  bool FillCache();
};

}  // namespace wz
#endif  // WZ_UTIL_WZBINARYREADER_H_

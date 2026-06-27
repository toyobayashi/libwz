#ifndef WZ_UTIL_WZSTREAM_H_
#define WZ_UTIL_WZSTREAM_H_

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <vector>

#include "wz/Util/Defines.h"

namespace wz {

enum class WzSeekOrigin {
  Begin,
  Current,
  End,
};

class WzStream {
 public:
  virtual ~WzStream() = default;
  virtual size_t Read(void* destination, size_t count) = 0;
  virtual bool Write(const void* data, size_t count) = 0;
  virtual bool Seek(int64_t offset, WzSeekOrigin origin) = 0;
  virtual int64_t Position() const = 0;
  virtual int64_t Size() const = 0;
  virtual bool Flush() = 0;
  virtual void Close() = 0;
  virtual bool IsOpen() const = 0;

  bool WriteByte(uint8_t value);
};

class WzFileStream final : public WzStream {
 public:
  WzFileStream() = default;
  WzFileStream(const std::filesystem::path& path, const char* mode);
  ~WzFileStream() override;

  WZ_DISALLOW_COPY(WzFileStream)
  WzFileStream(WzFileStream&& other) noexcept;
  WzFileStream& operator=(WzFileStream&& other) noexcept;

  bool Open(const std::filesystem::path& path, const char* mode);
  void Close() override;
  bool IsOpen() const override { return file_ != nullptr; }

  size_t Read(void* destination, size_t count) override;
  bool Write(const void* data, size_t count) override;
  bool Seek(int64_t offset, WzSeekOrigin origin) override;
  int64_t Position() const override;
  int64_t Size() const override { return size_; }
  bool Flush() override;

 private:
  FILE* file_ = nullptr;
  int64_t size_ = 0;

  bool RefreshSize();
};

class WzMemoryStream final : public WzStream {
 public:
  WzMemoryStream() = default;
  explicit WzMemoryStream(std::vector<uint8_t> data);

  size_t Read(void* destination, size_t count) override;
  bool Write(const void* data, size_t count) override;
  bool Seek(int64_t offset, WzSeekOrigin origin) override;
  int64_t Position() const override { return position_; }
  int64_t Size() const override { return static_cast<int64_t>(buffer_.size()); }
  bool Flush() override { return true; }
  void Close() override;
  bool IsOpen() const override { return true; }
  const std::vector<uint8_t>& Buffer() const { return buffer_; }
  std::vector<uint8_t>& Buffer() { return buffer_; }

 private:
  std::vector<uint8_t> buffer_;
  int64_t position_ = 0;
};

}  // namespace wz

#endif  // WZ_UTIL_WZSTREAM_H_

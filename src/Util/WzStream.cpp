#include "wz/Util/WzStream.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

namespace wz {

namespace {

FILE* OpenFilePath(const std::filesystem::path& path, const char* mode) {
#ifdef _WIN32
  std::wstring wideMode;
  for (const char* p = mode; *p != '\0'; ++p) {
    wideMode.push_back(static_cast<wchar_t>(*p));
  }
  return _wfopen(path.wstring().c_str(), wideMode.c_str());
#else
  return std::fopen(path.string().c_str(), mode);
#endif
}

int SeekOriginToC(WzSeekOrigin origin) {
  switch (origin) {
    case WzSeekOrigin::Begin:
      return SEEK_SET;
    case WzSeekOrigin::Current:
      return SEEK_CUR;
    case WzSeekOrigin::End:
      return SEEK_END;
  }
  return SEEK_SET;
}

bool FileSeek(FILE* file, int64_t offset, WzSeekOrigin origin) {
#ifdef _WIN32
  return _fseeki64(file, offset, SeekOriginToC(origin)) == 0;
#else
  return fseeko(file, static_cast<off_t>(offset), SeekOriginToC(origin)) == 0;
#endif
}

int64_t FileTell(FILE* file) {
#ifdef _WIN32
  return static_cast<int64_t>(_ftelli64(file));
#else
  return static_cast<int64_t>(ftello(file));
#endif
}

}  // namespace

bool WzStream::WriteByte(uint8_t value) {
  return Write(&value, sizeof(value));
}

WzFileStream::WzFileStream(const std::filesystem::path& path,
                           const char* mode) {
  Open(path, mode);
}

WzFileStream::~WzFileStream() {
  Close();
}

WzFileStream::WzFileStream(WzFileStream&& other) noexcept
    : file_(other.file_), size_(other.size_) {
  other.file_ = nullptr;
  other.size_ = 0;
}

WzFileStream& WzFileStream::operator=(WzFileStream&& other) noexcept {
  if (this != &other) {
    Close();
    file_ = other.file_;
    size_ = other.size_;
    other.file_ = nullptr;
    other.size_ = 0;
  }
  return *this;
}

bool WzFileStream::Open(const std::filesystem::path& path, const char* mode) {
  Close();
  file_ = OpenFilePath(path, mode);
  if (!file_) {
    size_ = 0;
    return false;
  }
  return RefreshSize();
}

void WzFileStream::Close() {
  if (file_) {
    std::fclose(file_);
    file_ = nullptr;
  }
  size_ = 0;
}

size_t WzFileStream::Read(void* destination, size_t count) {
  if (!file_ || count == 0) return 0;
  return std::fread(destination, 1, count, file_);
}

bool WzFileStream::Write(const void* data, size_t count) {
  if (!file_) return false;
  if (count == 0) return true;
  if (std::fwrite(data, 1, count, file_) != count) return false;
  const int64_t position = Position();
  if (position > size_) size_ = position;
  return true;
}

bool WzFileStream::Seek(int64_t offset, WzSeekOrigin origin) {
  return file_ && FileSeek(file_, offset, origin);
}

int64_t WzFileStream::Position() const {
  if (!file_) return -1;
  return FileTell(file_);
}

bool WzFileStream::Flush() {
  return file_ && std::fflush(file_) == 0;
}

bool WzFileStream::RefreshSize() {
  if (!file_) return false;
  const int64_t original = Position();
  if (original < 0) return false;
  if (!Seek(0, WzSeekOrigin::End)) return false;
  size_ = Position();
  return Seek(original, WzSeekOrigin::Begin);
}

WzMemoryStream::WzMemoryStream(std::vector<uint8_t> data)
    : buffer_(std::move(data)) {}

size_t WzMemoryStream::Read(void* destination, size_t count) {
  if (count == 0 || position_ < 0) return 0;
  const int64_t size = Size();
  if (position_ >= size) return 0;
  const size_t available = static_cast<size_t>(size - position_);
  const size_t toRead = std::min(count, available);
  std::memcpy(destination, buffer_.data() + position_, toRead);
  position_ += static_cast<int64_t>(toRead);
  return toRead;
}

bool WzMemoryStream::Write(const void* data, size_t count) {
  if (position_ < 0) return false;
  if (count == 0) return true;
  if (count >
      static_cast<size_t>(std::numeric_limits<int64_t>::max() - position_)) {
    return false;
  }
  const int64_t end = position_ + static_cast<int64_t>(count);
  if (end > Size()) {
    buffer_.resize(static_cast<size_t>(end));
  }
  std::memcpy(buffer_.data() + position_, data, count);
  position_ = end;
  return true;
}

bool WzMemoryStream::Seek(int64_t offset, WzSeekOrigin origin) {
  int64_t base = 0;
  switch (origin) {
    case WzSeekOrigin::Begin:
      base = 0;
      break;
    case WzSeekOrigin::Current:
      base = position_;
      break;
    case WzSeekOrigin::End:
      base = Size();
      break;
  }
  if ((offset > 0 && base > std::numeric_limits<int64_t>::max() - offset) ||
      (offset < 0 && base < std::numeric_limits<int64_t>::min() - offset)) {
    return false;
  }
  const int64_t next = base + offset;
  if (next < 0) return false;
  position_ = next;
  return true;
}

void WzMemoryStream::Close() {
  buffer_.clear();
  position_ = 0;
}

}  // namespace wz

#include "wz/Util/WzDataSource.h"

#include <algorithm>
#include <cstring>
#include <utility>

namespace wz {

WzFileDataSource::WzFileDataSource(WzFileStream* input)
    : input_(input),
      size_(input_ && input_->Size() > 0 ? static_cast<uint64_t>(input_->Size())
                                         : 0) {}

Result<size_t> WzFileDataSource::ReadAt(uint64_t offset,
                                        std::span<uint8_t> destination) {
  if (!input_ || !input_->IsOpen()) {
    return std::unexpected(Error::IoError("Source file is not open"));
  }
  if (offset > size_) {
    return std::unexpected(Error::IoError("Read offset is beyond source size"));
  }

  const uint64_t available = size_ - offset;
  const size_t count = static_cast<size_t>(
      std::min<uint64_t>(available, static_cast<uint64_t>(destination.size())));
  if (count == 0) return 0;

  if (!input_->Seek(static_cast<int64_t>(offset), WzSeekOrigin::Begin)) {
    return std::unexpected(Error::IoError("Failed to seek source file"));
  }

  if (input_->Read(destination.data(), count) != count) {
    return std::unexpected(Error::IoError("Failed to read source file"));
  }
  return count;
}

uint64_t WzFileDataSource::Size() const {
  return size_;
}

WzMemoryDataSource::WzMemoryDataSource(std::vector<uint8_t> data)
    : data_(std::move(data)) {}

Result<size_t> WzMemoryDataSource::ReadAt(uint64_t offset,
                                          std::span<uint8_t> destination) {
  if (offset > data_.size()) {
    return std::unexpected(Error::IoError("Read offset is beyond source size"));
  }

  const size_t index = static_cast<size_t>(offset);
  const size_t count = std::min(destination.size(), data_.size() - index);
  if (count > 0) std::memcpy(destination.data(), data_.data() + index, count);
  return count;
}

uint64_t WzMemoryDataSource::Size() const {
  return data_.size();
}

}  // namespace wz

#include "wz/Util/WzDataSource.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

namespace wz {

WzStreamDataSource::WzStreamDataSource(std::istream& input)
    : input_(&input), size_(0), is_valid_(false) {
  input_->clear();
  input_->seekg(0, std::ios::end);
  const std::istream::pos_type end = input_->tellg();
  if (end == std::istream::pos_type(-1)) {
    input_->clear();
    return;
  }
  size_ = static_cast<uint64_t>(end);
  input_->clear();
  input_->seekg(0, std::ios::beg);
  is_valid_ = input_->good();
}

Result<size_t> WzStreamDataSource::ReadAt(uint64_t offset,
                                          std::span<uint8_t> destination) {
  if (!is_valid_) {
    return std::unexpected(Error::IoError("Source stream is not seekable"));
  }
  if (offset > size_) {
    return std::unexpected(Error::IoError("Read offset is beyond source size"));
  }

  const uint64_t available = size_ - offset;
  const size_t count = static_cast<size_t>(
      std::min<uint64_t>(available, static_cast<uint64_t>(destination.size())));
  if (count == 0) return 0;
  if (count >
      static_cast<size_t>(std::numeric_limits<std::streamsize>::max())) {
    return std::unexpected(Error::IoError("Read size exceeds stream limits"));
  }
  if (offset >
      static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max())) {
    return std::unexpected(Error::IoError("Read offset exceeds stream limits"));
  }

  input_->clear();
  input_->seekg(static_cast<std::streamoff>(offset), std::ios::beg);
  if (!input_->good()) {
    return std::unexpected(Error::IoError("Failed to seek source stream"));
  }

  input_->read(reinterpret_cast<char*>(destination.data()),
               static_cast<std::streamsize>(count));
  if (input_->gcount() != static_cast<std::streamsize>(count)) {
    return std::unexpected(Error::IoError("Failed to read source stream"));
  }
  return count;
}

uint64_t WzStreamDataSource::Size() const {
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

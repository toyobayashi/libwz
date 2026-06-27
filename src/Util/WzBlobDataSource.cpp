#include "wz/Util/WzBlobDataSource.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace wz {

WzBlobDataSource::WzBlobDataSource(uint64_t size, ReadCallback read_callback)
    : size_(size), read_callback_(std::move(read_callback)) {}

Result<size_t> WzBlobDataSource::ReadAt(uint64_t offset,
                                        std::span<uint8_t> destination) {
  if (offset > size_) {
    return std::unexpected(Error::IoError("Read offset is beyond blob size"));
  }

  const uint64_t available = size_ - offset;
  const size_t count = static_cast<size_t>(
      std::min<uint64_t>(available, static_cast<uint64_t>(destination.size())));
  if (count == 0) return 0;
  if (count > static_cast<size_t>(std::numeric_limits<int>::max())) {
    return std::unexpected(Error::IoError("Blob read size exceeds limits"));
  }
  if (offset > 9007199254740991ULL) {
    return std::unexpected(Error::IoError("Blob read offset exceeds limits"));
  }

  if (!read_callback_) {
    return std::unexpected(Error::IoError("Blob read callback is not set"));
  }

  auto bytes_read = read_callback_(offset, destination.subspan(0, count));
  if (!bytes_read.has_value()) {
    return std::unexpected(bytes_read.error());
  }
  if (bytes_read.value() != count) {
    return std::unexpected(Error::IoError("Failed to read blob range"));
  }
  return bytes_read.value();
}

uint64_t WzBlobDataSource::Size() const {
  return size_;
}

}  // namespace wz

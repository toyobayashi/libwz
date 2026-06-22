#include "wz/Util/WzBlobDataSource.h"

#ifdef __EMSCRIPTEN__

#include <emscripten.h>

#include <algorithm>
#include <limits>

namespace wz {

namespace {

EM_JS(int,
      ReadBlobRange,
      (uint32_t blob_id, double offset, uint8_t* destination, int length),
      {
        const reader = globalThis.__libwzReadBlobRange;
        if (!(typeof reader == "function")) return -1;
        return reader(blob_id, offset, destination, length);
      });

}  // namespace

WzBlobDataSource::WzBlobDataSource(uint32_t blob_id, uint64_t size)
    : blob_id_(blob_id), size_(size) {}

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

  const int bytes_read = ReadBlobRange(blob_id_,
                                       static_cast<double>(offset),
                                       destination.data(),
                                       static_cast<int>(count));
  if (bytes_read < 0 || bytes_read != static_cast<int>(count)) {
    return std::unexpected(Error::IoError("Failed to read blob range"));
  }
  return static_cast<size_t>(bytes_read);
}

uint64_t WzBlobDataSource::Size() const {
  return size_;
}

}  // namespace wz

#endif  // __EMSCRIPTEN__

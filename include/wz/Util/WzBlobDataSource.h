#ifndef WZ_UTIL_WZBLOBDATASOURCE_H_
#define WZ_UTIL_WZBLOBDATASOURCE_H_

#ifdef __EMSCRIPTEN__

#include <cstdint>
#include <span>

#include "wz/Util/WzDataSource.h"

namespace wz {

class WzBlobDataSource final : public WzDataSource {
 public:
  WzBlobDataSource(uint32_t blob_id, uint64_t size);

  Result<size_t> ReadAt(uint64_t offset,
                        std::span<uint8_t> destination) override;
  uint64_t Size() const override;

 private:
  uint32_t blob_id_;
  uint64_t size_;
};

}  // namespace wz

#endif  // __EMSCRIPTEN__

#endif  // WZ_UTIL_WZBLOBDATASOURCE_H_

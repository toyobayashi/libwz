#ifndef WZ_UTIL_WZBLOBDATASOURCE_H_
#define WZ_UTIL_WZBLOBDATASOURCE_H_

#include <cstdint>
#include <functional>
#include <span>

#include "wz/Util/WzDataSource.h"

namespace wz {

class WzBlobDataSource final : public WzDataSource {
 public:
  using ReadCallback =
      std::function<Result<size_t>(uint64_t, std::span<uint8_t>)>;

  WzBlobDataSource(uint64_t size, ReadCallback read_callback);

  Result<size_t> ReadAt(uint64_t offset,
                        std::span<uint8_t> destination) override;
  uint64_t Size() const override;

 private:
  uint64_t size_;
  ReadCallback read_callback_;
};

}  // namespace wz

#endif  // WZ_UTIL_WZBLOBDATASOURCE_H_

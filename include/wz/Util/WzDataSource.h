#ifndef WZ_UTIL_WZDATASOURCE_H_
#define WZ_UTIL_WZDATASOURCE_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "wz/Result.h"
#include "wz/Util/WzStream.h"

namespace wz {

class WzDataSource {
 public:
  virtual ~WzDataSource() = default;
  virtual Result<size_t> ReadAt(uint64_t offset,
                                std::span<uint8_t> destination) = 0;
  virtual uint64_t Size() const = 0;
};

class WzFileDataSource final : public WzDataSource {
 public:
  explicit WzFileDataSource(WzFileStream* input);

  Result<size_t> ReadAt(uint64_t offset,
                        std::span<uint8_t> destination) override;
  uint64_t Size() const override;

 private:
  WzFileStream* input_ = nullptr;
  uint64_t size_;
};

class WzMemoryDataSource final : public WzDataSource {
 public:
  explicit WzMemoryDataSource(std::vector<uint8_t> data);

  Result<size_t> ReadAt(uint64_t offset,
                        std::span<uint8_t> destination) override;
  uint64_t Size() const override;

 private:
  std::vector<uint8_t> data_;
};

}  // namespace wz

#endif  // WZ_UTIL_WZDATASOURCE_H_

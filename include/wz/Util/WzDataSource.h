#ifndef WZ_UTIL_WZDATASOURCE_H_
#define WZ_UTIL_WZDATASOURCE_H_

#include <cstddef>
#include <cstdint>
#include <istream>
#include <span>
#include <vector>

#include "wz/Result.h"

namespace wz {

class WzDataSource {
 public:
  virtual ~WzDataSource() = default;
  virtual Result<size_t> ReadAt(uint64_t offset,
                                std::span<uint8_t> destination) = 0;
  virtual uint64_t Size() const = 0;
};

class WzStreamDataSource final : public WzDataSource {
 public:
  explicit WzStreamDataSource(std::istream& input);

  Result<size_t> ReadAt(uint64_t offset,
                        std::span<uint8_t> destination) override;
  uint64_t Size() const override;

 private:
  std::istream* input_;
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

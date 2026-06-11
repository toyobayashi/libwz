#ifndef WZ_UTIL_WZMUTABLEKEY_H_
#define WZ_UTIL_WZMUTABLEKEY_H_
#include <array>
#include <cstdint>
#include <vector>
#include "wz/Util/Defines.h"

namespace wz {

class WzMutableKey {
 public:
  static constexpr int BatchSize = 4096;

  WzMutableKey(const std::array<uint8_t, 4>& iv,
               const std::array<uint8_t, 32>& aesKey);
  WZ_DISALLOW_COPY(WzMutableKey)
  WzMutableKey(WzMutableKey&&) = default;
  WzMutableKey& operator=(WzMutableKey&&) = default;

  uint8_t operator[](size_t index) const;
  std::vector<uint8_t> GetKeys()
      const;  // returns a copy of the first 4 bytes of the key (the IV key)

  void EnsureKeySize(size_t size);

  const std::array<uint8_t, 4>& Iv() const { return iv_; }
  const std::array<uint8_t, 32>& AesKey() const { return aesKey_; }

  bool operator==(const WzMutableKey& other) const;
  bool operator!=(const WzMutableKey& other) const { return !(*this == other); }

 private:
  std::array<uint8_t, 4> iv_;
  std::array<uint8_t, 32> aesKey_;
  mutable std::vector<uint8_t> keys_;
};

}  // namespace wz
#endif  // WZ_UTIL_WZMUTABLEKEY_H_

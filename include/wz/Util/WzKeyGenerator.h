#ifndef WZ_UTIL_WZKEYGENERATOR_H_
#define WZ_UTIL_WZKEYGENERATOR_H_
#include <array>
#include <cstdint>
#include <memory>
#include <vector>
#include "wz/Util/WzMutableKey.h"

namespace wz {

// MapleCryptoConstants - default user key (128 bytes as 32 DWORDs)
extern const std::array<uint8_t, 128> MAPLESTORY_USERKEY_DEFAULT;

// Trimmed user key: takes 128-byte key, extracts 32 bytes (every 16th byte =
// first byte of each DWORD)
std::array<uint8_t, 32> GetTrimmedUserKey(
    const std::array<uint8_t, 128>& userKey);

class WzKeyGenerator {
 public:
  static WzMutableKey GenerateWzKey(const std::array<uint8_t, 4>& WzIv);
  static WzMutableKey GenerateLuaWzKey();
  static WzMutableKey GenerateWzKey(const std::array<uint8_t, 4>& WzIv,
                                    const std::array<uint8_t, 128>& AesUserKey);
};

}  // namespace wz
#endif  // WZ_UTIL_WZKEYGENERATOR_H_

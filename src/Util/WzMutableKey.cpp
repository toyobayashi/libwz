#include "wz/Util/WzMutableKey.h"
#include <cstring>
#include <aes.hpp>

namespace wz {

WzMutableKey::WzMutableKey(const std::array<uint8_t, 4>& iv,
                           const std::array<uint8_t, 32>& aesKey)
    : iv_(iv), aesKey_(aesKey) {}

uint8_t WzMutableKey::operator[](size_t index) const {
  const_cast<WzMutableKey*>(this)->EnsureKeySize(index + 1);
  return keys_[index];
}

std::vector<uint8_t> WzMutableKey::GetKeys() const {
  if (keys_.empty()) return {};
  return {keys_.begin(), keys_.begin() + std::min(size_t(4), keys_.size())};
}

void WzMutableKey::EnsureKeySize(size_t size) {
  if (!keys_.empty() && keys_.size() >= size) return;

  size = ((size + BatchSize - 1) / BatchSize) * BatchSize;
  std::vector<uint8_t> newKeys(size, 0);

  if (iv_[0] == 0 && iv_[1] == 0 && iv_[2] == 0 && iv_[3] == 0) {
    keys_ = std::move(newKeys);
    return;
  }

  size_t startIndex = 0;
  if (!keys_.empty()) {
    std::copy(keys_.begin(), keys_.end(), newKeys.begin());
    startIndex = keys_.size();
  }

  struct AES_ctx ctx;
  AES_init_ctx(&ctx, aesKey_.data());

  uint8_t block[16];
  for (size_t i = startIndex; i < size; i += 16) {
    if (i == 0) {
      for (int j = 0; j < 16; j++) block[j] = iv_[j % 4];
    } else {
      std::memcpy(block, newKeys.data() + i - 16, 16);
    }
    AES_ECB_encrypt(&ctx, block);
    std::memcpy(newKeys.data() + i, block, 16);
  }

  keys_ = std::move(newKeys);
}

bool WzMutableKey::operator==(const WzMutableKey& other) const {
  return iv_ == other.iv_ && aesKey_ == other.aesKey_;
}

}  // namespace wz

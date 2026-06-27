#ifndef WZ_PROPERTIES_WZLUAPROPERTY_H_
#define WZ_PROPERTIES_WZLUAPROPERTY_H_
#include <cstdint>
#include <memory>
#include <vector>
#include "wz/Util/WzMutableKey.h"
#include "wz/WzImageProperty.h"

namespace wz {
class WzLuaProperty : public WzImageProperty {
 public:
  WzLuaProperty(const std::string& name,
                const std::vector<uint8_t>& encryptedBytes);
  WZ_DISALLOW_COPY_AND_MOVE(WzLuaProperty)
  WzPropertyType PropertyType() const override { return WzPropertyType::Lua; }
  const std::vector<uint8_t>& Value() const { return encryptedBytes_; }
  Result<void> WriteValue(WzBinaryWriter* writer) const override;
  void SetValue(const std::vector<uint8_t>& value) override {
    if (encryptedBytes_ == value) return;
    encryptedBytes_ = value;
    MarkParentImageChanged();
  }
  std::string GetString() const override;
  std::vector<uint8_t> EncodeDecode(const std::vector<uint8_t>& input) const;

 private:
  std::vector<uint8_t> encryptedBytes_;
  WzMutableKey wzKey_;
};
}  // namespace wz
#endif  // WZ_PROPERTIES_WZLUAPROPERTY_H_

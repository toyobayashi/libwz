#include "wz/Properties/WzFloatProperty.h"
#include "wz/Util/WzBinaryWriter.h"

namespace wz {

Result<void> WzFloatProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(4);
  if (value_ == 0.0f) {
    writer->WriteByte(0);
  } else {
    writer->WriteByte(0x80);
    writer->WriteSingle(value_);
  }
  return {};
}

}  // namespace wz

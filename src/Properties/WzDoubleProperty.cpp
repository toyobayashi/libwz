#include "wz/Properties/WzDoubleProperty.h"
#include "wz/Util/WzBinaryWriter.h"

namespace wz {

Result<void> WzDoubleProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(5);
  writer->WriteDouble(value_);
  return {};
}

}  // namespace wz

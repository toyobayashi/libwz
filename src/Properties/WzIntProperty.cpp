#include "wz/Properties/WzIntProperty.h"
#include "wz/Util/WzBinaryWriter.h"

namespace wz {

Result<void> WzIntProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(3);
  writer->WriteCompressedInt(value_);
  return {};
}

}  // namespace wz

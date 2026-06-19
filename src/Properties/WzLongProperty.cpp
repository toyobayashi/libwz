#include "wz/Properties/WzLongProperty.h"
#include "wz/Util/WzBinaryWriter.h"

namespace wz {

Result<void> WzLongProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(20);
  writer->WriteCompressedLong(value_);
  return {};
}

}  // namespace wz

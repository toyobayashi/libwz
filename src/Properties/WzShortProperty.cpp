#include "wz/Properties/WzShortProperty.h"
#include "wz/Util/WzBinaryWriter.h"

namespace wz {

Result<void> WzShortProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(2);
  writer->WriteInt16(value_);
  return {};
}

}  // namespace wz

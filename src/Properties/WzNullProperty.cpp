#include "wz/Properties/WzNullProperty.h"
#include "wz/Util/WzBinaryWriter.h"

namespace wz {

Result<void> WzNullProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(0);
  return {};
}

}  // namespace wz

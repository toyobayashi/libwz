#include "wz/Properties/WzVectorProperty.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/WzImage.h"

namespace wz {

WzVectorProperty::~WzVectorProperty() = default;

Result<void> WzVectorProperty::WriteValue(WzBinaryWriter* writer) const {
  if (!X || !Y) {
    return std::unexpected(
        Error::DataError("Cannot write Vector property without X and Y"));
  }
  writer->WriteStringValue("Shape2D#Vector2D",
                           WzImage::WzImageHeaderByte_WithoutOffset,
                           WzImage::WzImageHeaderByte_WithOffset);
  writer->WriteCompressedInt(X->Value());
  writer->WriteCompressedInt(Y->Value());
  return {};
}

}  // namespace wz

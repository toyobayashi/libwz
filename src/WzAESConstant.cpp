#include "wz/WzAESConstant.h"

namespace wz {

const std::array<uint8_t, 4> WzAESConstant::WZ_GMSIV = {0x4D, 0x23, 0xC7, 0x2B};
const std::array<uint8_t, 4> WzAESConstant::WZ_MSEAIV = {
    0xB9, 0x7D, 0x63, 0xE9};
const std::array<uint8_t, 4> WzAESConstant::WZ_BMSCLASSIC = {
    0x00, 0x00, 0x00, 0x00};
const uint32_t WzAESConstant::WZ_OffsetConstant = 0x581C3F6D;

}  // namespace wz

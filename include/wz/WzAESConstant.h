#ifndef WZ_WZAESCONSTANT_H_
#define WZ_WZAESCONSTANT_H_
#include <array>
#include <cstdint>

namespace wz {

class WzAESConstant {
 public:
  static const std::array<uint8_t, 4> WZ_GMSIV;
  static const std::array<uint8_t, 4> WZ_MSEAIV;
  static const std::array<uint8_t, 4> WZ_BMSCLASSIC;
  static const uint32_t WZ_OffsetConstant;
};

}  // namespace wz
#endif  // WZ_WZAESCONSTANT_H_

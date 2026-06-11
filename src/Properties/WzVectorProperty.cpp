#include "wz/Properties/WzVectorProperty.h"

namespace wz {

WzVectorProperty::~WzVectorProperty() {
  delete X;
  X = nullptr;
  delete Y;
  Y = nullptr;
}

}  // namespace wz

#include "wz/Properties/WzStringProperty.h"
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include "wz/Util/WzBinaryWriter.h"

namespace wz {

Result<void> WzStringProperty::WriteValue(WzBinaryWriter* writer) const {
  writer->WriteByte(8);
  writer->WriteStringValue(value_, 0x00, 0x01);
  return {};
}

static int64_t safeParseInt64(const std::string& s) {
  char* end = nullptr;
  errno = 0;
  auto v = std::strtoll(s.c_str(), &end, 10);
  if (errno != 0 || end == s.c_str() || *end != '\0') return 0;
  return static_cast<int64_t>(v);
}

template <typename T>
static T safeParseInteger(const std::string& s) {
  int64_t value = safeParseInt64(s);
  if (value < std::numeric_limits<T>::min() ||
      value > std::numeric_limits<T>::max()) {
    return 0;
  }
  return static_cast<T>(value);
}

int32_t WzStringProperty::GetInt() const {
  return safeParseInteger<int32_t>(value_);
}
int16_t WzStringProperty::GetShort() const {
  return safeParseInteger<int16_t>(value_);
}
int64_t WzStringProperty::GetLong() const {
  return safeParseInt64(value_);
}

}  // namespace wz

#include "wz/Properties/WzStringProperty.h"
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include "wz/Util/WzPath.h"

namespace wz {

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

Result<void> WzStringProperty::SaveToFile(const std::string& filePath) {
  auto outPath = wz::to_path(filePath);
  auto parentPath = outPath.parent_path();
  std::error_code ec;
  if (!parentPath.empty()) {
    std::filesystem::create_directories(parentPath, ec);
    if (ec) return std::unexpected(Error::IoError(ec.message()));
  }
  std::ofstream out(outPath);
  if (!out)
    return std::unexpected(Error::IoError("Failed to open file for writing"));
  out << value_;
  return {};
}

}  // namespace wz

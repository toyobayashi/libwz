#include "wz/Properties/WzStringProperty.h"
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include "wz/Util/WzPath.h"

namespace wz {

static int32_t safeStoi(const std::string& s) {
  char* end = nullptr;
  errno = 0;
  auto v = std::strtol(s.c_str(), &end, 10);
  if (errno != 0 || end == s.c_str() || *end != '\0') return 0;
  return static_cast<int32_t>(v);
}

static int64_t safeStoll(const std::string& s) {
  char* end = nullptr;
  errno = 0;
  auto v = std::strtoll(s.c_str(), &end, 10);
  if (errno != 0 || end == s.c_str() || *end != '\0') return 0;
  return static_cast<int64_t>(v);
}

int32_t WzStringProperty::GetInt() const {
  return safeStoi(value_);
}
int16_t WzStringProperty::GetShort() const {
  return static_cast<int16_t>(safeStoi(value_));
}
int64_t WzStringProperty::GetLong() const {
  return safeStoll(value_);
}

bool WzStringProperty::SaveToFile(const std::string& filePath) {
  std::error_code ec;
  std::filesystem::create_directories(wz::to_path(filePath).parent_path(), ec);
  if (ec) return false;
  std::ofstream out(wz::to_path(filePath));
  if (!out) return false;
  out << value_;
  return true;
}

}  // namespace wz

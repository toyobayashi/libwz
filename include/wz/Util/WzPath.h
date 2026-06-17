#ifndef WZ_UTIL_WZPATH_H_
#define WZ_UTIL_WZPATH_H_

#include <filesystem>
#include <string>
#include "wz/Util/WzTool.h"

namespace wz {

// Converts a UTF-8 path string to std::filesystem::path.
// On Windows: MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS) first.
//   - If valid UTF-8 → convert to wstring → fs::path(wstring).
//   - If not valid UTF-8 (e.g. GBK on CJK Windows) → fall back to
//     fs::path(string) which uses the system ANSI code page (CP_ACP).
// On non-Windows: fs::path(string) — natively UTF-8, no conversion needed.
inline std::filesystem::path to_path(const std::string& utf8_path) {
#ifdef _WIN32
  return WzTool::ToPath(utf8_path);
#else
  return std::filesystem::path(utf8_path);
#endif
}

inline std::filesystem::path to_path(std::string&& utf8_path) {
#ifdef _WIN32
  return to_path(static_cast<const std::string&>(utf8_path));
#else
  return std::filesystem::path(std::move(utf8_path));
#endif
}

inline std::filesystem::path to_path(const char* utf8_path) {
  return to_path(std::string(utf8_path ? utf8_path : ""));
}

}  // namespace wz
#endif  // WZ_UTIL_WZPATH_H_

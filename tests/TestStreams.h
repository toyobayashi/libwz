#ifndef TESTS_TESTSTREAMS_H_
#define TESTS_TESTSTREAMS_H_

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzDataSource.h"
#include "wz/Util/WzPath.h"
#include "wz/Util/WzStream.h"

namespace test {

inline bool WriteFile(const std::filesystem::path& path,
                      const std::vector<uint8_t>& bytes) {
  wz::WzFileStream stream;
  return stream.Open(path, "wb") && stream.Write(bytes.data(), bytes.size()) &&
         stream.Flush();
}

inline bool WriteFile(const std::filesystem::path& path, const char* bytes) {
  wz::WzFileStream stream;
  size_t length = 0;
  while (bytes[length] != '\0') ++length;
  return stream.Open(path, "wb") && stream.Write(bytes, length) &&
         stream.Flush();
}

inline std::vector<uint8_t> ReadFile(const std::filesystem::path& path) {
  wz::WzFileStream stream;
  if (!stream.Open(path, "rb") || stream.Size() < 0) return {};
  std::vector<uint8_t> bytes(static_cast<size_t>(stream.Size()));
  if (!bytes.empty() &&
      stream.Read(bytes.data(), bytes.size()) != bytes.size()) {
    return {};
  }
  return bytes;
}

inline std::shared_ptr<wz::WzDataSource> MemorySource(
    const std::vector<uint8_t>& bytes) {
  return std::make_shared<wz::WzMemoryDataSource>(bytes);
}

}  // namespace test

#endif  // TESTS_TESTSTREAMS_H_

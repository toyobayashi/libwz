#include "wz/Util/WzTool.h"
#include <fstream>
#include <memory>
#include <string>
#include <unordered_set>
#include "wz/Util/WzPath.h"
#include "wz/WzAESConstant.h"
#include "wz/WzDirectory.h"
#include "wz/WzFile.h"
#include "wz/WzImage.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace wz {

namespace {

std::unordered_set<std::string> objectValueLengthCache;

}  // namespace

#ifdef _WIN32
std::filesystem::path WzTool::ToPath(const std::string& utf8_path) {
  if (utf8_path.empty()) return {};
  // MB_ERR_INVALID_CHARS: fails if the input is not valid UTF-8.
  // This lets us detect and fall back to ANSI (GBK etc.) gracefully.
  int n = MultiByteToWideChar(CP_UTF8,
                              MB_ERR_INVALID_CHARS,
                              utf8_path.c_str(),
                              static_cast<int>(utf8_path.size()),
                              nullptr,
                              0);
  if (n <= 0) {
    // Not valid UTF-8 — treat as system ANSI code page
    return std::filesystem::path(utf8_path);
  }
  std::wstring w(static_cast<size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8,
                      0,
                      utf8_path.c_str(),
                      static_cast<int>(utf8_path.size()),
                      &w[0],
                      n);
  return std::filesystem::path(w);
}
#endif

uint32_t WzTool::RotateLeft(uint32_t x, uint8_t n) {
  n &= 0x1F;
  if (n == 0) return x;
  return static_cast<uint32_t>((x << n) | (x >> (32 - n)));
}

uint32_t WzTool::RotateRight(uint32_t x, uint8_t n) {
  n &= 0x1F;
  if (n == 0) return x;
  return static_cast<uint32_t>((x >> n) | (x << (32 - n)));
}

int WzTool::GetCompressedIntLength(int i) {
  if (i > 127 || i < -127) return 5;
  return 1;
}

int WzTool::GetEncodedStringLength(const std::string& s) {
  if (s.empty()) return 1;

  bool unicode = false;
  for (unsigned char c : s) {
    if (c > 127) {
      unicode = true;
      break;
    }
  }

  int length = static_cast<int>(s.length());
  int prefixLength = (length > (unicode ? 126 : 127)) ? 5 : 1;
  int encodedLength = unicode ? length * 2 : length;
  return prefixLength + encodedLength;
}

int WzTool::GetWzObjectValueLength(const std::string& s, uint8_t type) {
  std::string storeName = std::to_string(type) + "_" + s;
  if (s.length() > 4 && objectValueLengthCache.contains(storeName)) {
    return 5;
  }
  objectValueLengthCache.insert(std::move(storeName));
  return 1 + GetEncodedStringLength(s);
}

void WzTool::ClearWzObjectValueLengthCache() {
  objectValueLengthCache.clear();
}

std::array<uint8_t, 4> WzTool::GetIvByMapleVersion(WzMapleVersion ver) {
  switch (ver) {
    case WzMapleVersion::EMS:
      return WzAESConstant::WZ_MSEAIV;
    case WzMapleVersion::GMS:
      return WzAESConstant::WZ_GMSIV;
    case WzMapleVersion::GENERATE:
      return {0, 0, 0, 0};
    case WzMapleVersion::BMS:
    case WzMapleVersion::CLASSIC:
    case WzMapleVersion::CUSTOM:
    default:
      return WzAESConstant::WZ_BMSCLASSIC;
  }
}

int WzTool::GetRecognizedCharacters(const std::string& source) {
  int count = 0;
  for (char c : source) {
    if (c >= 0x20 && c <= 0x7E) count++;
  }
  return count;
}

double WzTool::GetDecryptionSuccessRate(const std::string& wzPath,
                                        WzMapleVersion encVersion,
                                        int16_t* version) {
  std::unique_ptr<WzFile> wzf;
  if (version == nullptr || *version == 0) {
    wzf = std::make_unique<WzFile>(wzPath, encVersion);
  } else {
    wzf = std::make_unique<WzFile>(wzPath, *version, encVersion);
  }

  WzFileParseStatus parseStatus = wzf->ParseWzFile();
  if (parseStatus != WzFileParseStatus::Success) {
    return 0.0;
  }

  if (version) *version = wzf->Version();
  int recognizedChars = 0;
  int totalChars = 0;
  WzDirectory* wzDir = wzf->GetWzDirectory();
  if (wzDir) {
    for (WzDirectory* d : wzDir->WzDirectories()) {
      recognizedChars += GetRecognizedCharacters(d->Name());
      totalChars += static_cast<int>(d->Name().length());
    }
    for (WzImage* img : wzDir->WzImages()) {
      recognizedChars += GetRecognizedCharacters(img->Name());
      totalChars += static_cast<int>(img->Name().length());
    }
  }
  if (totalChars == 0) return 0.0;
  return static_cast<double>(recognizedChars) / static_cast<double>(totalChars);
}

WzMapleVersion WzTool::DetectMapleVersion(const std::string& wzFilePath,
                                          int16_t* fileVersion) {
  int16_t version = 0;
  double gmsRate =
      GetDecryptionSuccessRate(wzFilePath, WzMapleVersion::GMS, &version);
  double emsRate =
      GetDecryptionSuccessRate(wzFilePath, WzMapleVersion::EMS, &version);
  double bmsRate =
      GetDecryptionSuccessRate(wzFilePath, WzMapleVersion::BMS, &version);

  if (fileVersion) *fileVersion = version;

  WzMapleVersion bestVersion = WzMapleVersion::GMS;
  double maxRate = 0;
  if (gmsRate > maxRate) {
    maxRate = gmsRate;
    bestVersion = WzMapleVersion::GMS;
  }
  if (emsRate > maxRate) {
    maxRate = emsRate;
    bestVersion = WzMapleVersion::EMS;
  }
  if (bmsRate > maxRate) {
    maxRate = bmsRate;
    bestVersion = WzMapleVersion::BMS;
  }

  if (maxRate < 0.7) {
    std::string zlzPath =
        (wz::to_path(wzFilePath).parent_path() / "ZLZ.dll").string();
    if (std::filesystem::exists(wz::to_path(zlzPath)))
      return WzMapleVersion::GETFROMZLZ;
  }
  return bestVersion;
}

bool WzTool::IsListFile(const std::string& path) {
  std::ifstream reader(wz::to_path(path), std::ios::binary);
  if (!reader) return false;
  int32_t header = 0;
  reader.read(reinterpret_cast<char*>(&header), sizeof(header));
  return header != WZ_HEADER;
}

bool WzTool::IsDataWzHotfixFile(const std::string& path) {
  std::ifstream reader(wz::to_path(path), std::ios::binary);
  if (!reader) return false;
  char firstByte = 0;
  reader.read(&firstByte, 1);
  return static_cast<uint8_t>(firstByte) ==
         WzImage::WzImageHeaderByte_WithoutOffset;
}

}  // namespace wz

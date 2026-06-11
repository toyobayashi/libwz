#ifndef WZ_WZENUMS_H_
#define WZ_WZENUMS_H_
#include <cstdint>
#include <string>

namespace wz {

enum class WzObjectType : int {
  File = 0,
  Image = 1,
  Directory = 2,
  Property = 3,
  List = 4
};

enum class WzPropertyType : int {
  Null = 0,
  Short = 1,
  Int = 2,
  Long = 3,
  Float = 4,
  Double = 5,
  String = 6,
  SubProperty = 7,
  Canvas = 8,
  Vector = 9,
  Convex = 10,
  Sound = 11,
  Raw = 12,
  UOL = 13,
  Lua = 14,
  PNG = 15
};

enum class WzMapleVersion : int {
  GMS = 0,
  EMS = 1,
  BMS = 2,
  CLASSIC = 3,
  GENERATE = 4,
  GETFROMZLZ = 5,
  CUSTOM = 6,
  UNKNOWN = 99
};

enum class WzPngFormat : int {
  Format1 = 1,
  Format2 = 2,
  Format3 = 3,
  Format257 = 257,
  Format513 = 513,
  Format517 = 517,
  Format1026 = 1026,
  Format2050 = 2050
};

enum class WzDirectoryType : uint8_t {
  UnknownType_1 = 1,
  RetrieveStringFromOffset_2 = 2,
  WzDirectory_3 = 3,
  WzImage_4 = 4
};

enum class WzFileParseStatus : int {
  Path_Is_Null = -1,
  Error_Game_Ver_Hash = -2,
  Failed_Unknown = 0,
  Success = 1
};

enum class WzBinaryPropertyType : int { Raw = 0, MP3 = 1, WAV = 2 };

std::string GetErrorDescription(WzFileParseStatus status);

}  // namespace wz
#endif  // WZ_WZENUMS_H_

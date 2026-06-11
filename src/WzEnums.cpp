#include "wz/WzEnums.h"

namespace wz {

std::string GetErrorDescription(WzFileParseStatus status) {
  switch (status) {
    case WzFileParseStatus::Success:
      return "Success";
    case WzFileParseStatus::Failed_Unknown:
      return "Failed, in this case the causes are undetermined.";
    case WzFileParseStatus::Path_Is_Null:
      return "Path is null";
    case WzFileParseStatus::Error_Game_Ver_Hash:
      return "Error with game version hash : The specified game version is "
             "incorrect and WzLib was unable to determine the version itself";
  }
  return {};
}

}  // namespace wz

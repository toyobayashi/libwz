#ifndef WZ_PORTALTYPE_H_
#define WZ_PORTALTYPE_H_
#include <algorithm>
#include <string>
#include <unordered_map>
#include "wz/Result.h"

namespace wz {

enum class PortalType {
  StartPoint,
  Invisible,
  Visible,
  Default,
  Collision,
  Changeable,
  ChangeableInvisible,
  TownPortalPoint,
  Script,
  ScriptInvisible,
  CollisionScript,
  Hidden,
  ScriptHidden,
  CollisionVerticalJump,
  CollisionCustomImpact,
  CollisionCustomImpact2,
  CollisionUnknownPcig,
  ScriptHiddenUng,
  UNKNOWN_PCC,
  UNKNOWN_PCIR
};

class PortalTypeExtensions {
 public:
  static Result<std::string> ToCode(PortalType type) {
    auto it = portalTypeToCodes.find(type);
    if (it == portalTypeToCodes.end())
      return std::unexpected(Error::InvalidArgument("Unknown PortalType"));
    return it->second;
  }

  static Result<PortalType> FromCode(const std::string& code) {
    if (code.empty())
      return std::unexpected(Error::InvalidArgument("code is null"));
    std::string lower = code;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    auto it = codeToPortalTypes.find(lower);
    if (it == codeToPortalTypes.end())
      return std::unexpected(
          Error::InvalidArgument("Invalid portal type code: " + code));
    return it->second;
  }

  static Result<std::string> GetFriendlyName(PortalType type) {
    auto it = portalTypeToNames.find(type);
    if (it == portalTypeToNames.end())
      return std::unexpected(Error::InvalidArgument("Unknown PortalType"));
    return it->second;
  }

 private:
  static inline const std::unordered_map<PortalType, std::string>
      portalTypeToCodes = {
          {PortalType::StartPoint, "sp"},
          {PortalType::Invisible, "pi"},
          {PortalType::Visible, "pv"},
          {PortalType::Default, "default"},
          {PortalType::Collision, "pc"},
          {PortalType::Changeable, "pg"},
          {PortalType::ChangeableInvisible, "pgi"},
          {PortalType::TownPortalPoint, "tp"},
          {PortalType::Script, "ps"},
          {PortalType::ScriptInvisible, "psi"},
          {PortalType::CollisionScript, "pcs"},
          {PortalType::Hidden, "ph"},
          {PortalType::ScriptHidden, "psh"},
          {PortalType::CollisionVerticalJump, "pcj"},
          {PortalType::CollisionCustomImpact, "pci"},
          {PortalType::CollisionCustomImpact2, "pci2"},
          {PortalType::CollisionUnknownPcig, "pcig"},
          {PortalType::ScriptHiddenUng, "pshg"},
          {PortalType::UNKNOWN_PCC, "pcc"},
          {PortalType::UNKNOWN_PCIR, "pcir"},
  };

  static inline const std::unordered_map<std::string, PortalType>
      codeToPortalTypes = []() {
        std::unordered_map<std::string, PortalType> m;
        for (auto& p : portalTypeToCodes) {
          std::string lower = p.second;
          std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
          m[lower] = p.first;
        }
        return m;
      }();

  static inline const std::unordered_map<PortalType, std::string>
      portalTypeToNames = {
          {PortalType::StartPoint, "Start Point"},
          {PortalType::Invisible, "Invisible"},
          {PortalType::Visible, "Visible"},
          {PortalType::Default, "Visible (Default)"},
          {PortalType::Collision, "Collision"},
          {PortalType::Changeable, "Changeable"},
          {PortalType::ChangeableInvisible, "Changeable Invisible"},
          {PortalType::TownPortalPoint, "Town Portal"},
          {PortalType::Script, "Script"},
          {PortalType::ScriptInvisible, "Script Invisible"},
          {PortalType::CollisionScript, "Script Collision"},
          {PortalType::Hidden, "Hidden"},
          {PortalType::ScriptHidden, "Script Hidden"},
          {PortalType::CollisionVerticalJump, "Collision Vertical Jump"},
          {PortalType::CollisionCustomImpact, "Collision Custom Impact Spring"},
          {PortalType::CollisionCustomImpact2,
           "Collision Custom Impact Spring 2"},
          {PortalType::CollisionUnknownPcig, "Unknown (PCIG)"},
          {PortalType::ScriptHiddenUng, "Script Hidden Ung"},
          {PortalType::UNKNOWN_PCC, "Unknown pcc"},
          {PortalType::UNKNOWN_PCIR, "Unknown pcir"},
  };
};

}  // namespace wz
#endif  // WZ_PORTALTYPE_H_

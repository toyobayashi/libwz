#include "wz/Properties/WzUOLProperty.h"
#include <algorithm>
#include "wz/WzDirectory.h"
#include "wz/WzFile.h"
#include "wz/WzImage.h"

namespace wz {
WzUOLProperty::WzUOLProperty(const std::string& name, const std::string& value)
    : val_(value) {
  SetName(name);
}

WzObject* WzUOLProperty::ResolveLinkValue() {
  std::vector<std::string> paths;
  size_t start = 0, end;
  while ((end = val_.find('/', start)) != std::string::npos) {
    paths.push_back(val_.substr(start, end - start));
    start = end + 1;
  }
  paths.push_back(val_.substr(start));
  if (paths.empty()) return nullptr;

  // Determine starting point
  WzObject* linkVal;
  size_t i = 0;
  if (paths[0] != "..") {
    linkVal = GetTopMostWzImage();
  } else {
    linkVal = Parent();
    i = 1;  // skip first ".."
  }

  for (; i < paths.size() && linkVal; i++) {
    const std::string& path = paths[i];
    if (path == "..") {
      linkVal = linkVal->Parent();
      continue;
    }

    if (linkVal->ObjectType() == WzObjectType::Property) {
      auto* prop = static_cast<WzImageProperty*>(linkVal);
      linkVal = (*prop)[path];
    } else if (linkVal->ObjectType() == WzObjectType::Image) {
      auto* img = static_cast<WzImage*>(linkVal);
      linkVal = (*img)[path];
    } else if (linkVal->ObjectType() == WzObjectType::Directory) {
      auto* dir = static_cast<WzDirectory*>(linkVal);
      if (path.size() >= 4 &&
          std::equal(path.end() - 4, path.end(), ".img", [](char a, char b) {
            return std::tolower(a) == b;
          })) {
        linkVal = (*dir)[path];
      } else {
        linkVal = (*dir)[path + ".img"];
      }
    }
  }
  return linkVal;
}
}  // namespace wz

#include "wz/WzPropertyCollection.h"
#include "wz/WzImageProperty.h"

namespace wz {

void WzPropertyCollection::Add(WzImageProperty* item) {
  if (parent_) item->SetParent(parent_);
  items_.push_back(item);
}

void WzPropertyCollection::Insert(size_t index, WzImageProperty* item) {
  if (parent_) item->SetParent(parent_);
  items_.insert(items_.begin() + static_cast<ptrdiff_t>(index), item);
}

}  // namespace wz

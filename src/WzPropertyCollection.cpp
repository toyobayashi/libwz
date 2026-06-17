#include "wz/WzPropertyCollection.h"
#include "wz/WzImageProperty.h"

namespace wz {

void WzPropertyCollection::Add(WzImageProperty* item) {
  Add(std::unique_ptr<WzImageProperty>(item));
}

void WzPropertyCollection::Add(std::unique_ptr<WzImageProperty> item) {
  if (parent_) item->SetParent(parent_);
  items_.push_back(std::move(item));
}

void WzPropertyCollection::Insert(size_t index, WzImageProperty* item) {
  Insert(index, std::unique_ptr<WzImageProperty>(item));
}

void WzPropertyCollection::Insert(size_t index,
                                  std::unique_ptr<WzImageProperty> item) {
  if (parent_) item->SetParent(parent_);
  items_.insert(items_.begin() + static_cast<ptrdiff_t>(index),
                std::move(item));
}

}  // namespace wz

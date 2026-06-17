#include "wz/WzPropertyCollection.h"
#include "wz/WzImageProperty.h"

namespace wz {

WzPropertyCollection::WzPropertyCollection() = default;

WzPropertyCollection::WzPropertyCollection(WzObject* parent)
    : parent_(parent) {}

WzPropertyCollection::~WzPropertyCollection() = default;

WzPropertyCollection::WzPropertyCollection(
    WzPropertyCollection&& other) noexcept
    : items_(std::move(other.items_)), parent_(other.parent_) {}

WzPropertyCollection& WzPropertyCollection::operator=(
    WzPropertyCollection&& other) noexcept {
  items_ = std::move(other.items_);
  parent_ = other.parent_;
  return *this;
}

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

void WzPropertyCollection::erase(iterator it) {
  items_.erase(it.it_);
}

void WzPropertyCollection::erase_at(size_t index) {
  items_.erase(items_.begin() + static_cast<ptrdiff_t>(index));
}

void WzPropertyCollection::clear() {
  items_.clear();
}

std::vector<std::unique_ptr<WzImageProperty>>
WzPropertyCollection::TakeItems() {
  return std::move(items_);
}

}  // namespace wz

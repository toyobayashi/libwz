#include "wz/WzPropertyCollection.h"
#include <algorithm>
#include <utility>
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
  if (it.it_ != items_.end() && it.it_->get())
    it.it_->get()->SetParent(nullptr);
  items_.erase(it.it_);
}

void WzPropertyCollection::erase_at(size_t index) {
  auto it = items_.begin() + static_cast<ptrdiff_t>(index);
  if (it->get()) it->get()->SetParent(nullptr);
  items_.erase(it);
}

std::unique_ptr<WzImageProperty> WzPropertyCollection::Take(
    WzImageProperty* item) {
  auto it =
      std::find_if(items_.begin(), items_.end(), [item](const auto& current) {
        return current.get() == item;
      });
  if (it == items_.end()) return nullptr;

  auto result = std::move(*it);
  items_.erase(it);
  if (result) result->SetParent(nullptr);
  return result;
}

void WzPropertyCollection::clear() {
  for (auto& item : items_) {
    if (item) item->SetParent(nullptr);
  }
  items_.clear();
}

std::vector<std::unique_ptr<WzImageProperty>>
WzPropertyCollection::TakeItems() {
  return std::move(items_);
}

}  // namespace wz

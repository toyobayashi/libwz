#ifndef WZ_WZPROPERTYCOLLECTION_H_
#define WZ_WZPROPERTYCOLLECTION_H_
#include <memory>
#include <utility>
#include <vector>
#include "wz/WzObject.h"

namespace wz {

class WzImageProperty;
class WzPropertyCollection {
 public:
  WzPropertyCollection() = default;
  explicit WzPropertyCollection(WzObject* parent) : parent_(parent) {}
  WZ_DISALLOW_COPY(WzPropertyCollection)
  WzPropertyCollection(WzPropertyCollection&& other) noexcept
      : items_(std::move(other.items_)), parent_(other.parent_) {}
  WzPropertyCollection& operator=(WzPropertyCollection&& other) noexcept {
    items_ = std::move(other.items_);
    parent_ = other.parent_;
    return *this;
  }

  void Add(WzImageProperty* item);

  void Insert(size_t index, WzImageProperty* item);

  void erase(std::vector<WzImageProperty*>::iterator it) { items_.erase(it); }
  void clear() { items_.clear(); }

  WzImageProperty*& operator[](size_t i) { return items_[i]; }
  WzImageProperty* operator[](size_t i) const { return items_[i]; }

  size_t size() const { return items_.size(); }

  auto begin() { return items_.begin(); }
  auto end() { return items_.end(); }
  auto begin() const { return items_.begin(); }
  auto end() const { return items_.end(); }

  WzObject* Parent() const { return parent_; }

 private:
  std::vector<WzImageProperty*> items_;
  WzObject* parent_ = nullptr;
};

}  // namespace wz
#endif  // WZ_WZPROPERTYCOLLECTION_H_

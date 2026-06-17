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
  class iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = WzImageProperty*;
    using difference_type = std::ptrdiff_t;
    using pointer = WzImageProperty**;
    using reference = WzImageProperty*;

    iterator() = default;
    explicit iterator(
        std::vector<std::unique_ptr<WzImageProperty>>::iterator it)
        : it_(it) {}

    WzImageProperty* operator*() const { return it_->get(); }
    iterator& operator++() {
      ++it_;
      return *this;
    }
    iterator operator++(int) {
      iterator old = *this;
      ++(*this);
      return old;
    }
    bool operator==(const iterator& other) const { return it_ == other.it_; }
    bool operator!=(const iterator& other) const { return !(*this == other); }

   private:
    friend class WzPropertyCollection;
    std::vector<std::unique_ptr<WzImageProperty>>::iterator it_;
  };

  class const_iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = WzImageProperty*;
    using difference_type = std::ptrdiff_t;
    using pointer = WzImageProperty* const*;
    using reference = WzImageProperty*;

    const_iterator() = default;
    explicit const_iterator(
        std::vector<std::unique_ptr<WzImageProperty>>::const_iterator it)
        : it_(it) {}

    WzImageProperty* operator*() const { return it_->get(); }
    const_iterator& operator++() {
      ++it_;
      return *this;
    }
    const_iterator operator++(int) {
      const_iterator old = *this;
      ++(*this);
      return old;
    }
    bool operator==(const const_iterator& other) const {
      return it_ == other.it_;
    }
    bool operator!=(const const_iterator& other) const {
      return !(*this == other);
    }

   private:
    std::vector<std::unique_ptr<WzImageProperty>>::const_iterator it_;
  };

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
  void Add(std::unique_ptr<WzImageProperty> item);

  void Insert(size_t index, WzImageProperty* item);
  void Insert(size_t index, std::unique_ptr<WzImageProperty> item);

  void erase(iterator it) { items_.erase(it.it_); }
  void erase_at(size_t index) {
    items_.erase(items_.begin() + static_cast<ptrdiff_t>(index));
  }
  void clear() { items_.clear(); }
  std::vector<std::unique_ptr<WzImageProperty>> TakeItems() {
    return std::move(items_);
  }

  WzImageProperty* operator[](size_t i) { return items_[i].get(); }
  WzImageProperty* operator[](size_t i) const { return items_[i].get(); }

  size_t size() const { return items_.size(); }

  iterator begin() { return iterator(items_.begin()); }
  iterator end() { return iterator(items_.end()); }
  const_iterator begin() const { return const_iterator(items_.begin()); }
  const_iterator end() const { return const_iterator(items_.end()); }

  WzObject* Parent() const { return parent_; }

 private:
  std::vector<std::unique_ptr<WzImageProperty>> items_;
  WzObject* parent_ = nullptr;
};

}  // namespace wz
#endif  // WZ_WZPROPERTYCOLLECTION_H_

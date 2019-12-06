#pragma once

#include <cassert>
#include <cstring>
#include "allocator.hh"

namespace kipper {
namespace internal {

template <class T, class Alloc = Allocator>
class List {
 public:
  explicit List(int capacity);

  ~List() {
    Deallocate(items_, capacity_);
    Invalid();
  }

  List(const List&) = delete;
  List& operator=(const List&) = delete;

  List(List&& that);

  List& operator=(List&& that);

  void Add(const T& value);

  void RemoveLast();

  bool Resize(int capacity);

  T& ReleaseLast();

  T& operator[](int index) const;

  T& First() const { return operator[](0); }

  T& Last() const { return operator[](size() - 1); }

  int capacity() const { return capacity_; }

  T* data() const { return items_; }

  int size() const { return size_; }

 private:
  static T* Allocate(int capacity) {
    if (capacity) {
      return reinterpret_cast<T*>(Alloc::AllocateArray(sizeof(T), capacity));
    }
    return nullptr;
  }

  static void Deallocate(T* items, size_t capacity) {
    if (items) {
      Alloc::DeallocateArray(items, sizeof(T), capacity);
    }
  }

  void EnsureSize() {
    if (capacity_ <= size_) {
      Resize(capacity_ + 1 + (capacity_ >> 1));
    }
  }

  void Invalid() {
    capacity_ = -1;
    items_ = nullptr;
    size_ = -1;
  }

  int capacity_;
  T* items_;
  int size_ = 0;
};

template <class T, class Alloc>
inline List<T, Alloc>::List(int capacity)
    : capacity_{capacity}, items_{Allocate(capacity_)} {}

template <class T, class Alloc>
inline List<T, Alloc>::List(List&& that)
    : capacity_{that.capacity_}, items_(that.items_), size_(that.size_) {
  that.Invalid();
}

template <class T, class Alloc>
List<T, Alloc>& List<T, Alloc>::operator=(List<T, Alloc>&& that) {
  capacity_ = that.capacity_;
  items_ = that.items_;
  size_ = that.size_;
  that.Invalid();
  return *this;
}

template <class T, class Alloc>
void List<T, Alloc>::Add(const T& value) {
  assert(size_ >= 0 && capacity_ >= 0);
  EnsureSize();
  assert(capacity_ > size_);
  items_[size_++] = value;
}

template <class T, class Alloc>
void List<T, Alloc>::RemoveLast() {
  size_--;
  int recapacity = (capacity_ >> 2) * 3;
  if (recapacity > size_) {
    Resize(recapacity);
  }
}

template <class T, class Alloc>
bool List<T, Alloc>::Resize(int capacity) {
  auto old_capacity = capacity_;
  capacity_ = capacity;
  auto resized_items = Allocate(capacity_);
  memcpy(resized_items, items_, size_ * sizeof(T));
  Deallocate(items_, old_capacity * sizeof(T));
  items_ = resized_items;
  return true;
}

template <class T, class Alloc>
inline T& List<T, Alloc>::ReleaseLast() {
  assert(size() > 0);
  T& result = Last();
  size_--;
  return result;
}

template <class T, class Alloc>
inline T& List<T, Alloc>::operator[](int index) const {
  assert(size_ >= 0 && capacity_ >= 0);
  assert(index < size_);
  return items_[index];
}

}  // namespace internal
}  // namespace kipper
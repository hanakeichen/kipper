#pragma once

#include "kipper.hh"
#include "list.hh"

namespace kipper {
namespace internal {

class HeapObject;
class Object;

struct Space {
  Address begin() const { return start; }
  Address end() const { return start + size; }

  size_t FreeSize() const { return end() - free; }

  Address start;
  Address free;
  size_t size;
  size_t available_objects{0};
};

class NewSpace : public Space {
 public:
  NewSpace(Address start, size_t semispace_size)
      : Space{start, start, semispace_size << 1, 0},
        semispace_size_{semispace_size},
        to_space_{start},
        from_space_{start + semispace_size} {}

  ~NewSpace() { from_space_ = to_space_ = nullptr; }

  Address Allocate(size_t size);

  void Flip() {
    std::swap(from_space_, to_space_);
    free = to_space_;

    available_objects = 0;
  }

  bool Contains(Address addr) const { return addr >= begin() && addr < end(); }

  bool IsInFrom(Address addr) const {
    return addr >= FromSpaceLow() && addr < FromSpaceHigh();
  }

  void PrintObjects();

  size_t semispace_size() const { return semispace_size_; }

  Address FromSpaceLow() const { return from_space_; }

  Address FromSpaceHigh() const { return from_space_ + semispace_size_; }

  Address ToSpaceLow() const { return to_space_; }

  Address ToSpaceHigh() const { return to_space_ + semispace_size_; }

 private:
  size_t semispace_size_;
  Address from_space_;
  Address to_space_;
};

class OldSpace : public Space {
 public:
  class RSetIterator {
   public:
    RSetIterator(List<HeapObject*>* rset, int index)
        : rset_{rset}, index_{index} {}

    RSetIterator& operator++() {
      index_++;
      return *this;
    }

    RSetIterator operator++(int) {
      auto result = *this;
      ++*this;
      return result;
    }

    bool operator==(const RSetIterator& other) const {
      return rset_ == other.rset_ && index_ == other.index_;
    }

    bool operator!=(const RSetIterator& other) const {
      return !(*this == other);
    }

    HeapObject* operator*() { return (*rset_)[index_]; }

    HeapObject** location() { return &(*rset_)[index_]; }

    void SwapLastForRemove() {
      (*rset_)[index_] = rset_->Last();
      rset_->ReleaseLast();
    }

   private:
    List<HeapObject*>* rset_;
    int index_;
  };

  OldSpace(Address start, size_t size)
      : Space{start, start, size, 0}, rset_{0} {}

  Address Allocate(size_t size);

  bool Contains(Address addr) const { return addr >= begin() && addr < end(); }

  RSetIterator RSetBegin() { return RSetIterator{&rset_, 0}; }

  RSetIterator RSetEnd() { return RSetIterator{&rset_, rset_.size()}; }

  void RememberObject(HeapObject* obj);

  void RemoveRoot(int r_index);

  void PrintObjects();

 private:
  List<HeapObject*> rset_;

  friend class RSetIterator;
};

class MetadataSpace : public Space {
 public:
  MetadataSpace(Address start, size_t size) : Space{start, start, size} {}

  void* Allocate(size_t size);
};

inline Address NewSpace::Allocate(size_t size) {
  if (free + size > ToSpaceHigh()) {
    return nullptr;
  }

  available_objects++;

  auto result = free;
  free += size;
  return result;
}

inline Address OldSpace::Allocate(size_t size) {
  if (free + size > end()) {
    return nullptr;
  }

  available_objects++;

  auto result = free;
  free += size;
  return result;
}

inline void* MetadataSpace::Allocate(size_t size) {
  if (free + size > end()) {
    return nullptr;
  }
  auto result = free;
  free += size;
  return result;
}

}  // namespace internal
}  // namespace kipper
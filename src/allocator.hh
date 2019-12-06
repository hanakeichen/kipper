#pragma once

#include <cstdlib>
#include "kipper.hh"

namespace kipper {
namespace internal {

class Allocator : public AllStatic {
 public:
  static void* Allocate(size_t size);

  static void Deallocate(void* p, size_t size);

  static void* AllocateArray(size_t element_size, size_t capacity);

  static void DeallocateArray(void* p, size_t element_size, size_t capacity);

  static size_t AllocateSize() { return allocate_size_; }

 private:
  static size_t allocate_size_;
};

}  // namespace internal
}  // namespace kipper
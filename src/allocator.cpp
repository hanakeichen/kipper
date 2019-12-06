#include "allocator.hh"
#include <cassert>
#include <cstdlib>

using namespace kipper::internal;

size_t Allocator::allocate_size_{0};

void* Allocator::Allocate(size_t size) {
  assert(size > 0);
  allocate_size_ += size;
  return std::malloc(size);
}

void Allocator::Deallocate(void* p, size_t size) {
  assert(allocate_size_ >= 0);
  std::free(p);
  allocate_size_ -= size;
}

void* Allocator::AllocateArray(size_t element_size, size_t capacity) {
  assert(element_size > 0 && capacity > 0);
  allocate_size_ += element_size * capacity;
  return std::malloc(element_size * capacity);
}

void Allocator::DeallocateArray(void* p, size_t element_size, size_t capacity) {
  assert(element_size > 0 && capacity > 0);
  std::free(p);
  allocate_size_ -= element_size * capacity;
}
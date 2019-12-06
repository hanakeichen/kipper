#include "handle.hh"
#include "list.hh"
#include "value.hh"

using namespace kipper::internal;

static constexpr int kHandleSize = 1 * KB;

HandleArea HandleScope::current_ = {nullptr, nullptr, 0};
HandleScope::HandleList HandleScope::handles_ = HandleList{0};

inline void** AllocateHandleChunk() {
  return static_cast<void**>(
      Allocator::AllocateArray(kPointerSize, kHandleSize));
}

inline void DeallocateHandleChunk(void** chunk) {
  Allocator::DeallocateArray(chunk, kPointerSize, kHandleSize);
}

void** HandleScope::MakeHandle(void* value) {
  void** handle = current_.handle;
  if (handle == current_.end) {
    handle = AllocateHandleChunk();
    handles_.Add(handle);
    current_.handle = handle;
    current_.end = handle + kHandleSize;
    current_.chunks++;
  }
  *handle = value;
  current_.handle++;
  return handle;
}

void HandleScope::IterateHandles(ObjectVisitor* visitor) {
  if (current_.handle) {
    for (auto obj_it = handles_.Last(); obj_it != current_.handle; obj_it++) {
      visitor->Visit(reinterpret_cast<Object**>(obj_it));
    }
    for (auto chunk_i = 0, len = handles_.size() - 1; chunk_i < len;
         chunk_i++) {
      for (auto obj_it = handles_[chunk_i],
                obj_end = handles_[chunk_i] + kHandleSize;
           obj_it != obj_end; obj_it++) {
        visitor->Visit(reinterpret_cast<Object**>(obj_it));
      }
    }
  }
}

void HandleScope::Exit() {
  for (auto i = 0; i < current_.chunks; i++) {
    Allocator::DeallocateArray(handles_.ReleaseLast(), kPointerSize,
                               kHandleSize);
  }
  current_ = prev_;
}
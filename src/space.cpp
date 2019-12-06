#include "space.hh"
#include "allocator.hh"
#include "log.hh"
#include "value.hh"

using namespace kipper::internal;

void NewSpace::PrintObjects() {
  for (auto it = ToSpaceLow(); it != free;) {
    auto obj = HeapObject::Make(it);
    LOG_DEBUG("NewSpace::PrintObject address: {} metadata: {:b}, type: {:b}",
              static_cast<void*>(obj), obj->metadata().EncodedMetadata(),
              obj->metadata().Type());
    it += obj->Size();
  }
}

void OldSpace::RememberObject(HeapObject* obj) {
  assert(obj->IsHeapObject());
  auto heap_obj = HeapObject::Cast(obj);
  assert(Contains(heap_obj->address()));
  auto metadata = heap_obj->metadata();
  if (metadata.IsRemembered()) {
    return;
  }
  rset_.Add(obj);
  metadata.Remember();
  heap_obj->set_metadata(metadata);
}

void OldSpace::RemoveRoot(int r_index) {
  rset_[r_index] = rset_.Last();
  rset_.ReleaseLast();
}

void OldSpace::PrintObjects() {
  for (auto it = start; it != free;) {
    auto obj = HeapObject::Make(it);
    LOG_DEBUG("OldSpace::PrintObject address: {} metadata: {:b}, type: {:b}",
              static_cast<void*>(obj), obj->metadata().EncodedMetadata(),
              obj->metadata().Type());
    LOG_DEBUG("to_string: {}", obj->ToStdString());
    it += obj->Size();
  }
}

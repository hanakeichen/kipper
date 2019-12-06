#include "heap.hh"
#include "allocator.hh"
#include "context.hh"
#include "gc.hh"
#include "handle.hh"
#include "space.hh"
#include "value.hh"

using namespace kipper::internal;

#define ADDRESS(p) reinterpret_cast<Address>(p)

#define ROOT_LIST_DEF(T, name) T *Heap::name##_{nullptr};
ROOT_LIST(ROOT_LIST_DEF)
#undef ROOT_LIST_DEF

#define ALLOC_WITH_GC_SUPPORT(ALLOC_FUNC)      \
  try {                                        \
    return ALLOC_FUNC;                         \
  } catch (const AllocationError &alloc_err) { \
    Heap::Collect(alloc_err.space());          \
    try {                                      \
      return ALLOC_FUNC;                       \
    } catch (const AllocationError &) {        \
      throw OutOfMemoryError{};                \
    }                                          \
  }

Address Heap::heap_start_ = nullptr;
NewSpace Heap::new_space_{nullptr, 0};
OldSpace Heap::old_space_{nullptr, 0};

Context *Heap::global_context_ = nullptr;
SymbolTable Heap::symbol_table_;

size_t Heap::young_space_size_ = 0;
size_t Heap::semispace_size_ = 256 * KB;
size_t Heap::old_space_size_ = 16 * MB;
uint8_t Heap::tenure_threshold_ = 2;
bool Heap::initialized_ = false;

class RootVerifyObjectVisitor : public ObjectVisitor {
 public:
  void Visit(Object **handle) override {
    if ((*handle)->IsHeapObject()) {
      auto heap_obj = HeapObject::Cast(*handle);
      if (Heap::IsInNewSpace(heap_obj)) {
        assert(!Heap::new_space()->IsInFrom(heap_obj->address()));
      } else if (Heap::IsInOldSpace(heap_obj)) {
        assert(heap_obj->address() >= Heap::old_space()->begin() &&
               heap_obj->address() < Heap::old_space()->end());
      }

      heap_obj->IterateBody(this);
    }
  }
};

void Heap::Configure(size_t heap_size, uint8_t tenure_threshold) {
  if (IsInitialized()) {
    return;
  }
  auto semispace_size = heap_size >> 2U;
  auto old_size = heap_size >> 1U;
  if (semispace_size > 0) {
    semispace_size_ = NextPowerOf2(semispace_size);
  }
  if (old_size > 0) {
    old_space_size_ = NextPowerOf2(old_size);
  }
  young_space_size_ = semispace_size_ << 1U;
  tenure_threshold_ = tenure_threshold;
}

void Heap::Initialize() {
  if (IsInitialized()) {
    return;
  }
  heap_start_ = ADDRESS(Allocator::Allocate(TotalSize()));

  new_space_ = NewSpace{heap_start_, semispace_size_};
  old_space_ = OldSpace{new_space_.end(), old_space_size_};

  global_context_ = new Context{nullptr};
  InitializeRootList();

  initialized_ = true;
}

inline bool Heap::IsInitialized() { return initialized_; }

void Heap::Shutdown() {
  new_space_.~NewSpace();

  Allocator::Deallocate(heap_start_, TotalSize());

  delete global_context_;

  initialized_ = false;
}

Context *Heap::GlobalContext() { return global_context_; }

HeapObject *Heap::AllocateKSObject(int elements_size, AllocationPolicy policy) {
  ALLOC_WITH_GC_SUPPORT(AllocateKSObjectNoGC(elements_size, policy));
}

HeapObject *Heap::AllocateString(int32_t length, AllocationPolicy policy) {
  ALLOC_WITH_GC_SUPPORT(AllocateStringNoGC(length, policy));
}

HeapObject *Heap::AllocateArray(int32_t length, AllocationPolicy policy) {
  ALLOC_WITH_GC_SUPPORT(AllocateArrayNoGC(length, policy));
}

HeapObject *Heap::AllocateArrayNoGC(int32_t length, AllocationPolicy policy) {
  return length ? AllocateArrayNoGCInternal(length, policy) : empty_array();
}

HeapObject *Heap::AllocateKSArray(int32_t length, AllocationPolicy policy) {
  ALLOC_WITH_GC_SUPPORT(AllocateKSArrayNoGC(length, policy));
}

HeapObject *Heap::AllocateHeapNumber(AllocationPolicy policy) {
  ALLOC_WITH_GC_SUPPORT(AllocateHeapNumberNoGC(policy));
}

HeapObject *Heap::AllocateFunction(AllocationPolicy policy) {
  ALLOC_WITH_GC_SUPPORT(AllocateFunctionNoGC(policy));
}

HeapObject *Heap::AllocateHashTable(int32_t elements_size,
                                    AllocationPolicy policy) {
  ALLOC_WITH_GC_SUPPORT(AllocateHashTableNoGC(elements_size, policy));
}

HeapObject *Heap::AllocateHashTableNoGC(int32_t elements_size,
                                        AllocationPolicy policy) {
  return elements_size ? AllocateHashTableNoGCInternal(elements_size, policy)
                       : empty_hash_table();
}

HeapObject *Heap::LookupSymbol(std::string_view symbol) {
  auto search = symbol_table_.Find(symbol);
  if (search) {
    return search;
  }
  auto result = String::Cast(AllocateSymbol(symbol));
  symbol_table_.Insert(result);
  return result;
}

bool Heap::IsInNewSpace(Object *obj) {
  return obj->IsHeapObject() &&
         new_space_.Contains(HeapObject::Cast(obj)->address());
}

bool Heap::IsInOldSpace(HeapObject *obj) {
  return old_space_.Contains(obj->address());
}

void Heap::IterateRoots(ObjectVisitor *visitor) {
  Context::IterateContext(visitor);
  HandleScope::IterateHandles(visitor);

#define ROOT_ITERATE(T, name) \
  visitor->Visit(reinterpret_cast<Object **>(&name##_));
  ROOT_LIST(ROOT_ITERATE)
#undef ROOTS_ITERATE
}

void Heap::IterateSymbolTable(ObjectVisitor *visitor) {
  for (auto &it : symbol_table_) {
    visitor->Visit(reinterpret_cast<Object **>(it->symbol_handle()));
  }
}

void Heap::WriteBarrier(HeapObject *obj, Object *field) {
  if (old_space_.Contains(obj->address()) && field->IsHeapObject() &&
      new_space()->Contains(HeapObject::Cast(field)->address())) {
    old_space_.RememberObject(obj);
  }
}

HeapObject *Heap::AllocateRaw(size_t size, AllocationSpace space) {
  Address result = nullptr;
  switch (space) {
    case NEW_SPACE:
      result = new_space_.Allocate(size);
      break;
    case OLD_SPACE:
      result = old_space_.Allocate(size);
      break;
  }
  if (result) {
    return HeapObject::Make(result);
  }
  throw AllocationError{space};
}

HeapObject *Heap::Promote(HeapObject *obj) {
  assert(IsInNewSpace(obj));
  auto result = AllocateRaw(obj->Size(), AllocationSpace::OLD_SPACE);
  std::memcpy(result->address(), obj->address(), obj->Size());
  auto metadata = obj->metadata();
  metadata.Forwarding(result->address());
  obj->set_metadata(metadata);
  CopyingCollector::AddPromotedObject(result);
  return result;
}

HeapObject *Heap::CopyObject(HeapObject *from_obj) {
  assert(from_obj->IsHeapObject());
  if (!IsInNewSpace(from_obj)) {
    return from_obj;
  }
  auto from_metadata = from_obj->metadata();
  if (from_metadata.IsForwarding()) {
    return from_metadata.Forwarding();
  }
  assert(new_space_.IsInFrom(from_obj->address()));
  if (from_metadata.Age() >= tenure_threshold()) {
    try {
      return Promote(from_obj);
    } catch (const AllocationError &) {
      // Do nothing
    }
  }
  auto result = AllocateRaw(from_obj->Size(), AllocationSpace::NEW_SPACE);
  memcpy(result->address(), from_obj->address(), from_obj->Size());
  auto result_metadata = result->metadata();
  result_metadata.IncrementAge();
  result->set_metadata(result_metadata);
  from_metadata.Forwarding(result->address());
  from_obj->set_metadata(from_metadata);
  return result;
}

void Heap::CleanupSymbolTable() { symbol_table_.Cleanup(); }

void Heap::Collect(AllocationSpace space) {
  VerifyHeapObjects();

  if (space == AllocationSpace::NEW_SPACE) {
    GCStats::StartYoungGC();
    CopyingCollector::Collect();
    GCStats::StopYoungGC();
  } else {
    if (new_space_.free == new_space_.ToSpaceHigh()) {
      GCStats::StartFullGC();
      MarkCompactCollector::Collect();
      CopyingCollector::Collect();
      GCStats::StopFullGC();
    } else {
      GCStats::StartOldGC();
      MarkCompactCollector::Collect();
      GCStats::StopOldGC();
    }
  }

  VerifyHeapObjects();
}

void Heap::InitializeRootList() {
  empty_array_ = AllocateArrayNoGCInternal(0, TENURED);
  empty_hash_table_ = AllocateHashTableNoGCInternal(0, TENURED);
  empty_string_ = AllocateStringNoGCInternal(0, TENURED);

#define ROOT_LIST_VERIFY(T, name) assert(name##_ != nullptr);
  ROOT_LIST(ROOT_LIST_VERIFY)
#undef ROOT_LIST_VERIFY
}

HeapObject *Heap::AllocateStringNoGC(int32_t length, AllocationPolicy policy) {
  return length ? AllocateStringNoGCInternal(length, policy) : empty_string();
}

inline HeapObject *Heap::AllocateKSObjectNoGC(int32_t elements_size,
                                              AllocationPolicy policy) {
  return AllocateKSObjectNoGCInternal(elements_size, policy);
}

inline HeapObject *Heap::AllocateKSArrayNoGC(int32_t length,
                                             AllocationPolicy policy) {
  return AllocateKSArrayNoGCInternal(length, policy);
}

HeapObject *Heap::AllocateHeapNumberNoGC(AllocationPolicy policy) {
  auto space = policy == NOT_TENURED ? NEW_SPACE : OLD_SPACE;
  auto result = AllocateRaw(HeapNumber::kSize, space);
  InitializeMetadata(result, HeapObjectType::HEAP_NUMBER);
  return result;
}

HeapObject *Heap::AllocateFunctionNoGC(AllocationPolicy policy) {
  auto space = policy == NOT_TENURED ? NEW_SPACE : OLD_SPACE;
  auto result = AllocateRaw(Function::kSize, space);
  InitializeMetadata(result, HeapObjectType::FUNCTION);
  return result;
}

HeapObject *Heap::AllocateArrayNoGCInternal(int32_t length,
                                            AllocationPolicy policy) {
  auto size = Array::EnsureSize(length);
  auto space = policy == NOT_TENURED ? NEW_SPACE : OLD_SPACE;
  auto result = AllocateRaw(size, space);
  InitializeMetadata(result, HeapObjectType::ARRAY);
  auto array_result = Array::Cast(result);
  array_result->SetLength(length);
  for (int i = 0; i < length; i++) {
    array_result->Set(i, Constant::Undefined());
  }
  return result;
}

HeapObject *Heap::AllocateHashTableNoGCInternal(int32_t elements_size,
                                                AllocationPolicy policy) {
  int capacity = NextPowerOf2(elements_size);
  if (capacity < 2) {
    capacity = 2;
  }
  auto table = HashTable::Cast(
      Heap::AllocateArrayNoGC(HashTable::EnsureSize(capacity), policy));
  table->SetElementsSize(0);
  table->SetCapacity(capacity);
  return table;
}

HeapObject *Heap::AllocateStringNoGCInternal(int32_t length,
                                             AllocationPolicy policy) {
  auto size = String::EnsureSize(length);
  auto space = policy == NOT_TENURED ? NEW_SPACE : OLD_SPACE;
  auto result = AllocateRaw(size, space);
  InitializeMetadata(result, HeapObjectType::STRING);
  String::Cast(result)->Length(length);
  KSObject::Cast(result)->SetElements(HashTable::Cast(empty_hash_table()));
  return result;
}

HeapObject *Heap::AllocateKSObjectNoGCInternal(int32_t elements_size,
                                               AllocationPolicy policy) {
  auto space = policy == NOT_TENURED ? NEW_SPACE : OLD_SPACE;
  auto properties = AllocateHashTableNoGC(elements_size, policy);
  auto result = AllocateRaw(KSObject::kSize, space);
  InitializeMetadata(result, HeapObjectType::KSOBJECT);
  KSObject::Cast(result)->SetElements(HashTable::Cast(properties));
  return result;
}

HeapObject *Heap::AllocateKSArrayNoGCInternal(int32_t length,
                                              AllocationPolicy policy) {
  auto elements = Array::Cast(AllocateArrayNoGC(length, policy));
  auto space = policy == NOT_TENURED ? NEW_SPACE : OLD_SPACE;
  auto result = AllocateRaw(KSArray::kSize, space);
  InitializeMetadata(result, HeapObjectType::KSARRAY);
  KSArray::Cast(result)->SetElements(elements);
  KSArray::Cast(result)->SetLength(length);
  KSObject::Cast(result)->SetElements(HashTable::Cast(empty_hash_table()));
  return result;
}

HeapObject *Heap::AllocateSymbol(std::string_view symbol) {
  String *result = String::Cast(AllocateString(symbol.size(), TENURED));
  result->Content(symbol);
  return result;
}

void Heap::InitializeMetadata(HeapObject *obj, HeapObjectType type) {
  Metadata metadata{nullptr};
  metadata.set_type(type);
  obj->set_metadata(metadata);
}

void Heap::VerifyHeapObjects() {
  RootVerifyObjectVisitor verifier_visitor;
  IterateRoots(&verifier_visitor);
}

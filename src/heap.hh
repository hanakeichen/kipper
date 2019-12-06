#pragma once

#include <string_view>
#include "kipper.hh"
#include "symbol_table.hh"
#include "value.hh"

namespace kipper::internal {

#define ROOT_LIST(K)              \
  K(HeapObject, empty_array)      \
  K(HeapObject, empty_hash_table) \
  K(HeapObject, empty_string)

class Context;
class NewSpace;
class OldSpace;

enum AllocationSpace { NEW_SPACE, OLD_SPACE };

class Heap : public AllStatic {
 public:
  static void Configure(size_t heap_size, uint8_t tenure_threshold);

  static void Initialize();

  static bool IsInitialized();

  static void Shutdown();

  static Context* GlobalContext();

  static HeapObject* AllocateKSObject(int elements_size,
                                      AllocationPolicy policy = NOT_TENURED);

  static HeapObject* AllocateString(int32_t length,
                                    AllocationPolicy policy = NOT_TENURED);

  static HeapObject* AllocateArray(int32_t length,
                                   AllocationPolicy policy = NOT_TENURED);

  static HeapObject* AllocateArrayNoGC(int32_t length,
                                       AllocationPolicy policy = NOT_TENURED);

  static HeapObject* AllocateKSArray(int32_t length,
                                     AllocationPolicy policy = NOT_TENURED);

  static HeapObject* AllocateHeapNumber(AllocationPolicy policy = NOT_TENURED);

  static HeapObject* AllocateFunction(AllocationPolicy policy = NOT_TENURED);

  static HeapObject* AllocateHashTable(int32_t elements_size,
                                       AllocationPolicy policy = NOT_TENURED);

  static HeapObject* AllocateHashTableNoGC(
      int32_t elements_size, AllocationPolicy policy = NOT_TENURED);

  static HeapObject* LookupSymbol(std::string_view symbol);

  static bool IsInNewSpace(Object* obj);

  static bool IsInOldSpace(HeapObject* obj);

  static void IterateRoots(ObjectVisitor* visitor);

  static void IterateSymbolTable(ObjectVisitor* visitor);

  static void WriteBarrier(HeapObject* obj, Object* field);

  static HeapObject* AllocateRaw(size_t size, AllocationSpace space);

  [[nodiscard]] static HeapObject* Promote(HeapObject* obj);

  [[nodiscard]] static HeapObject* CopyObject(HeapObject* from_obj);

  static void CleanupSymbolTable();

  static void Collect(AllocationSpace space);

  static size_t TotalSize() { return young_space_size_ + old_space_size_; }

  static NewSpace* new_space() { return &new_space_; }

  static OldSpace* old_space() { return &old_space_; }

  static uint8_t tenure_threshold() { return tenure_threshold_; }

 private:
  static void InitializeRootList();

  static HeapObject* AllocateStringNoGC(int32_t length,
                                        AllocationPolicy policy);

  static HeapObject* AllocateKSObjectNoGC(int32_t elements_size,
                                          AllocationPolicy policy);

  static HeapObject* AllocateKSArrayNoGC(int32_t length,
                                         AllocationPolicy policy);

  static HeapObject* AllocateHeapNumberNoGC(AllocationPolicy policy);

  static HeapObject* AllocateFunctionNoGC(AllocationPolicy policy);

  static HeapObject* AllocateArrayNoGCInternal(int32_t length,
                                               AllocationPolicy policy);

  static HeapObject* AllocateHashTableNoGCInternal(int32_t elements_size,
                                                   AllocationPolicy policy);

  static HeapObject* AllocateStringNoGCInternal(int32_t length,
                                                AllocationPolicy policy);

  static HeapObject* AllocateKSObjectNoGCInternal(int32_t elements_size,
                                                  AllocationPolicy policy);

  static HeapObject* AllocateKSArrayNoGCInternal(int32_t length,
                                                 AllocationPolicy policy);

  static HeapObject* AllocateSymbol(std::string_view symbol);

  static void InitializeMetadata(HeapObject* obj, HeapObjectType type);

  static void VerifyHeapObjects();

  static Address heap_start_;
  static NewSpace new_space_;
  static OldSpace old_space_;

  static Context* global_context_;
  static SymbolTable symbol_table_;

#define ROOT_LIST_DECL(T, name) static T* name##_;
  ROOT_LIST(ROOT_LIST_DECL)
#undef ROOT_LIST_DECL

#define ROOT_LIST_GETTER(T, name) \
  static T* name() { return name##_; }
  ROOT_LIST(ROOT_LIST_GETTER)
#undef ROOT_LIST_GETTER

  static size_t semispace_size_;
  static size_t young_space_size_;
  static size_t old_space_size_;
  static uint8_t tenure_threshold_;
  static bool initialized_;
};

class AllocationError : public KError {
 public:
  explicit AllocationError(AllocationSpace space)
      : KError{"Allocates failed"}, space_{space} {}

  AllocationSpace space() const { return space_; }

 private:
  AllocationSpace space_;
};

class OutOfMemoryError : public KError {
 public:
  OutOfMemoryError() : KError{"Out of memory"} {}
};

}  // namespace kipper::internal
#pragma once

#include <chrono>
#include "allocator.hh"
#include "kipper.hh"
#include "value.hh"

namespace kipper {
namespace internal {

class CopyingCollector : public AllStatic {
 public:
  static void Collect();

  static void AddPromotedObject(HeapObject* promoted_obj);

  DISABLE_DEFAULT_OP(CopyingCollector)
 private:
  static void Copying();

  static void WriteBarrier(HeapObject* obj);

  static HeapObject** prompted_offset_;
};

class MarkCompactCollector : public AllStatic {
 public:
  static void Collect();

  DISABLE_DEFAULT_OP(MarkCompactCollector)
 private:
  static void Mark();
  static void Compact();
  static void SetForwarding();
  static void AdjustPtr();
  static void MoveObject();

  static void CleanupSymbolTable();
};

class GCStats : public AllStatic {
 public:
  static void StartYoungGC();

  static void StopYoungGC();

  static void StartOldGC();

  static void StopOldGC();

  static void StartFullGC();

  static void StopFullGC();

 private:
  static void LogHeapInfo();

  static std::chrono::system_clock::time_point gc_start_time_;
  static std::chrono::nanoseconds young_gc_time_;
  static std::chrono::nanoseconds old_gc_time_;
  static std::chrono::nanoseconds full_gc_time_;
  static size_t young_gc_count_;
  static size_t old_gc_count_;
  static size_t full_gc_count_;
};

}  // namespace internal
}  // namespace kipper
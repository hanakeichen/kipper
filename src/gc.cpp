#include "gc.hh"
#include <cassert>
#include "context.hh"
#include "heap.hh"
#include "log.hh"
#include "space.hh"

using namespace kipper::internal;

HeapObject** CopyingCollector::prompted_offset_{nullptr};
std::chrono::system_clock::time_point GCStats::gc_start_time_;
std::chrono::nanoseconds GCStats::young_gc_time_;
std::chrono::nanoseconds GCStats::old_gc_time_;
std::chrono::nanoseconds GCStats::full_gc_time_;
size_t GCStats::young_gc_count_;
size_t GCStats::old_gc_count_;
size_t GCStats::full_gc_count_;

class CopyObjectVisitor : public ObjectVisitor {
 public:
  void Visit(Object** handle) override {
    if (Heap::IsInNewSpace(*handle)) {
      Visit(reinterpret_cast<HeapObject**>(handle));
    }
  }

  void Visit(HeapObject** handle) {
    assert((*handle)->IsHeapObject());
    auto prev_obj = *handle;
    *handle = Heap::CopyObject(*handle);
  }
};

class RSetCopyObjectVisitor : public ObjectVisitor {
 public:
  void Visit(Object** handle) override {
    RSetChildrenCopyObjectVisitor child_copy_visitor;
    HeapObject::Cast(*handle)->IterateBody(&child_copy_visitor);
    is_robj_remove_ = child_copy_visitor.is_robj_remove();
  }

  bool is_robj_remove() const { return is_robj_remove_; }

  void set_is_robj_remove(bool is_robj_remove) {
    is_robj_remove_ = is_robj_remove;
  }

 private:
  class RSetChildrenCopyObjectVisitor : public ObjectVisitor {
   public:
    void Visit(Object** handle) override {
      if (Heap::IsInNewSpace(*handle)) {
        *handle = Heap::CopyObject(HeapObject::Cast(*handle));
        if (!is_robj_remove_) {
          return;
        }
        is_robj_remove_ = !Heap::IsInNewSpace(*handle);
      }
    }

    bool is_robj_remove() const { return is_robj_remove_; }

    void set_is_robj_remove(bool is_robj_remove) {
      is_robj_remove_ = is_robj_remove;
    }

   private:
    bool is_robj_remove_{true};
  };

  bool is_robj_remove_{true};
};

class BarrierObjectVisitor : public ObjectVisitor {
 public:
  void Visit(Object** handle) override {
    if (is_write_barrier_) {
      return;
    }
    if ((*handle)->IsHeapObject() &&
        Heap::new_space()->Contains(HeapObject::Cast(*handle)->address())) {
      is_write_barrier_ = true;
    }
  }

  bool is_write_barrier() const { return is_write_barrier_; }

 private:
  bool is_write_barrier_{false};
};

class MarkObjectVisitor : public ObjectVisitor {
 public:
  void Visit(Object** handle) override {
    if ((*handle)->IsHeapObject() &&
        Heap::IsInOldSpace(HeapObject::Cast(*handle))) {
      MarkObject(reinterpret_cast<HeapObject**>(handle));
    }
  }

 private:
  void MarkObject(HeapObject** handle) {
    auto obj = *handle;
    assert(obj->IsHeapObject() && Heap::IsInOldSpace(obj));
    auto metadata = obj->metadata();
    if (metadata.IsMarked()) {
      return;
    }
    metadata.Mark();
    obj->set_metadata(metadata);
    obj->IterateBody(this);
  }
};

class AdjustPtrVisitor : public ObjectVisitor {
 public:
  void Visit(Object** handle) override {
    if ((*handle)->IsHeapObject()) {
      if (auto obj = HeapObject::Cast(*handle); Heap::IsInOldSpace(obj)) {
        *handle = obj->metadata().Forwarding();
      }
    }
  }
};

class RSetAdjustPtrObjectVisitor : public ObjectVisitor {
 public:
  void Visit(Object** handle) override {
    if ((*handle)->IsHeapObject()) {
      if (auto obj = HeapObject::Cast(*handle); Heap::IsInOldSpace(obj)) {
        if (obj->metadata().IsMarked()) {
          *handle = obj->metadata().Forwarding();
          is_robj_remove_ = false;
        } else {
          is_robj_remove_ = true;
        }
      }
    }
  }

  bool is_robj_remove() const { return is_robj_remove_; }

  void set_is_robj_remove(bool is_robj_remove) {
    is_robj_remove_ = is_robj_remove;
  }

 private:
  bool is_robj_remove_{false};
};

#ifndef NDEBUG

class RootsInToSpaceVerifier : public ObjectVisitor {
 public:
  RootsInToSpaceVerifier() { Heap::IterateRoots(this); }

  void Visit(Object** handle) override {
    if (Heap::IsInNewSpace(*handle)) {
      assert(HeapObject::Cast(*handle)->address() >=
                 Heap::new_space()->ToSpaceLow() &&
             HeapObject::Cast(*handle)->address() <
                 Heap::new_space()->ToSpaceHigh());
    }
    if ((*handle)->IsHeapObject()) {
      HeapObject::Cast(*handle)->IterateBody(this);
    }
  }
};

class RootsInFromSpaceVerifier : public ObjectVisitor {
 public:
  RootsInFromSpaceVerifier() { Heap::IterateRoots(this); }

  void Visit(Object** handle) override {
    if (Heap::IsInNewSpace(*handle)) {
      assert(HeapObject::Cast(*handle)->address() >=
                 Heap::new_space()->FromSpaceLow() &&
             HeapObject::Cast(*handle)->address() <
                 Heap::new_space()->FromSpaceHigh());
    }
    if ((*handle)->IsHeapObject()) {
      HeapObject::Cast(*handle)->IterateBody(this);
    }
  }
};

#endif  // !NDEBUG

void CopyingCollector::Collect() {
#ifndef NDEBUG
  { RootsInToSpaceVerifier{}; }
#endif

  Heap::new_space()->Flip();

#ifndef NDEBUG
  { RootsInFromSpaceVerifier{}; }
#endif

  Copying();
}

void CopyingCollector::AddPromotedObject(HeapObject* promoted_obj) {
  prompted_offset_--;
  *prompted_offset_ = promoted_obj;
}

template <class SPACE, class RSET_VISITOR>
static void IterateRSet(SPACE* space, RSET_VISITOR* visitor) {
  auto it = space->RSetBegin();
  while (it != space->RSetEnd()) {
    assert((*it)->IsHeapObject() && Heap::IsInOldSpace(*it));
    visitor->set_is_robj_remove(true);
    visitor->Visit(reinterpret_cast<Object**>(it.location()));
    if (visitor->is_robj_remove()) {
      auto metadata = HeapObject::Cast(*it)->metadata();
      metadata.ResetRemembered();
      HeapObject::Cast(*it)->set_metadata(metadata);
      it.SwapLastForRemove();
    } else {
      it++;
    }
  }
}

// Cheney's algorithm
void CopyingCollector::Copying() {
  Address scan = Heap::new_space()->ToSpaceLow();
  auto promoted_top =
      reinterpret_cast<HeapObject**>(Heap::new_space()->ToSpaceHigh());
  prompted_offset_ = reinterpret_cast<HeapObject**>(promoted_top);

  CopyObjectVisitor copy_visitor;
  Heap::IterateRoots(&copy_visitor);

  RSetCopyObjectVisitor rset_copy_visitor;
  IterateRSet(Heap::old_space(), &rset_copy_visitor);
  while (true) {
    while (scan != Heap::new_space()->free) {
      auto current = HeapObject::Make(scan);
      current->IterateBody(&copy_visitor);
      scan = scan + current->Size();
    }
    assert(scan <= reinterpret_cast<Address>(prompted_offset_));
    if (prompted_offset_ < promoted_top) {
      do {
        promoted_top--;
        auto obj = (*promoted_top);
        obj->IterateBody(&copy_visitor);
        WriteBarrier(obj);
      } while (prompted_offset_ < promoted_top);
      continue;
    }
    break;
  }
}

void CopyingCollector::WriteBarrier(HeapObject* obj) {
  assert(Heap::IsInOldSpace(obj));
  if (obj->IsArray()) {
    auto array = Array::Cast(obj);
    for (auto i = 0, len = array->Length(); i < len; i++) {
      if (Heap::IsInNewSpace(array->Get(i))) {
        Heap::old_space()->RememberObject(obj);
        break;
      }
    }
  } else {
    BarrierObjectVisitor visitor;
    obj->IterateBody(&visitor);
    if (visitor.is_write_barrier()) {
      Heap::old_space()->RememberObject(obj);
    }
  }
}

void MarkCompactCollector::Collect() {
  Mark();
  Compact();
}

void MarkCompactCollector::Mark() {
  MarkObjectVisitor mark_object_visitor;
  Heap::IterateRoots(&mark_object_visitor);
  CleanupSymbolTable();
}

void MarkCompactCollector::Compact() {
  SetForwarding();
  AdjustPtr();
  MoveObject();
}

void MarkCompactCollector::SetForwarding() {
  auto new_addr = Heap::old_space()->begin();
  auto scan = new_addr;
  while (scan < Heap::old_space()->free) {
    auto obj = HeapObject::Make(scan);
    auto metadata = obj->metadata();
    if (metadata.IsMarked()) {
      metadata.Forwarding(new_addr);
      obj->set_metadata(metadata);
      new_addr += obj->Size();
    }
    scan += obj->Size();
  }
}

void MarkCompactCollector::AdjustPtr() {
  AdjustPtrVisitor adjust_ptr_visitor;
  Heap::IterateRoots(&adjust_ptr_visitor);
  Heap::IterateSymbolTable(&adjust_ptr_visitor);

  RSetAdjustPtrObjectVisitor rset_adjust_ptr_visitor;
  IterateRSet(Heap::old_space(), &rset_adjust_ptr_visitor);

  auto scan = Heap::old_space()->begin();
  while (scan < Heap::old_space()->free) {
    auto obj = HeapObject::Make(scan);
    if (obj->metadata().IsMarked()) {
      obj->IterateBody(&adjust_ptr_visitor);
    }
    scan += obj->Size();
  }
}

void MarkCompactCollector::MoveObject() {
  auto free = Heap::old_space()->begin();
  auto scan = free;

  int available_objects = 0;

  while (scan < Heap::old_space()->free) {
    auto obj = HeapObject::Make(scan);
    auto metadata = obj->metadata();
    auto obj_size = obj->Size();
    if (metadata.IsMarked()) {
      auto new_addr_obj = metadata.Forwarding();
      memcpy(new_addr_obj->address(), obj->address(), obj_size);
      auto new_metadata = new_addr_obj->metadata();
      new_metadata.ResetForwarding();
      new_metadata.ResetMarked();
      new_addr_obj->set_metadata(new_metadata);
      free += obj_size;
      available_objects++;
    }
    scan += obj_size;
  }
  Heap::old_space()->free = free;

  Heap::old_space()->available_objects = available_objects;
}

void MarkCompactCollector::CleanupSymbolTable() { Heap::CleanupSymbolTable(); }

void GCStats::StartYoungGC() {
  LogHeapInfo();
  LOG_DEBUG("Young GC start...");
  young_gc_count_++;
  gc_start_time_ = std::chrono::system_clock::now();
}

void GCStats::StopYoungGC() {
  auto gc_end_time = std::chrono::system_clock::now();
  auto cost = gc_end_time - gc_start_time_;
  young_gc_time_ += cost;
  LOG_DEBUG(
      "Young GC stop... cost: {}ms",
      std::chrono::duration_cast<std::chrono::milliseconds>(cost).count());
  LogHeapInfo();
}

void GCStats::StartOldGC() {
  LogHeapInfo();
  LOG_DEBUG("Old GC start...");
  old_gc_count_++;
  gc_start_time_ = std::chrono::system_clock::now();
}

void GCStats::StopOldGC() {
  auto gc_end_time = std::chrono::system_clock::now();
  auto cost = gc_end_time - gc_start_time_;
  old_gc_time_ += cost;
  LOG_DEBUG(
      "Old GC stop... cost: {}ms",
      std::chrono::duration_cast<std::chrono::milliseconds>(cost).count());
  LogHeapInfo();
}

void GCStats::StartFullGC() {
  LogHeapInfo();
  LOG_DEBUG("Full GC start...");
  full_gc_count_++;
  gc_start_time_ = std::chrono::system_clock::now();
}

void GCStats::StopFullGC() {
  auto gc_end_time = std::chrono::system_clock::now();
  auto cost = gc_end_time - gc_start_time_;
  full_gc_time_ += cost;
  LOG_DEBUG(
      "Full GC stop... cost: {}ms",
      std::chrono::duration_cast<std::chrono::milliseconds>(cost).count());
  LogHeapInfo();
}

inline void GCStats::LogHeapInfo() {
  LOG_DEBUG(
      "new space avaliable objects: {}, old space "
      "avaliable objects: {}, number of young gc events: {}, number of old gc "
      "events: {}, number of full gc events: {}",
      Heap::new_space()->available_objects,
      Heap::old_space()->available_objects, young_gc_count_, old_gc_count_,
      full_gc_count_);
}

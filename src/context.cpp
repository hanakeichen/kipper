#include "context.hh"
#include "heap.hh"
#include "value.hh"

using namespace kipper::internal;

#define RESOLVE_CONTEXT(_context, name)                                 \
  do {                                                                  \
    for (auto i = (_context)->chunks_.size() - 2; i > 0; i -= 2) {      \
      if (*reinterpret_cast<String**>((_context)->chunks_.data()[i]) == \
          (name).get()) {                                               \
        return Handle((_context)->chunks_.data()[i + 1]);               \
      }                                                                 \
    }                                                                   \
  } while (false)

static constexpr int kContextChunkLimit = 1 << 4;

inline static Object** AllocateVarChunk() {
  return static_cast<Object**>(
      Allocator::AllocateArray(kPointerSize, kContextChunkLimit));
}

inline static void DeallocateVarChunk(Object** chunk) {
  return Allocator::DeallocateArray(chunk, kPointerSize, kContextChunkLimit);
}

Context::Context(Context* parent)
    : parent_{parent},
      next_{nullptr},
      chunks_{List<Object**>{0}},
      chunk_start_{nullptr},
      chunk_end_{nullptr} {
  if (parent_) {
    parent_->next_ = this;
  }
}

Context::~Context() {
  for (auto i = 0; i < chunks_.size(); i++) {
    DeallocateVarChunk(chunks_[i]);
  }
  parent_->next_ = nullptr;
}

Handle<Object> Context::Resolve(String* name) {
  if (auto result = Search(name); result) {
    return result;
  }
  return Handle<Object>{};
}

Handle<Object> Context::Push(String* name, Object* value) {
  auto result = SearchCurrentContext(name);
  if (result) {
    *result.location() = value;
    return result;
  }
  if (chunk_start_ == chunk_end_) {
    chunk_start_ = AllocateVarChunk();
    chunks_.Add(chunk_start_);
    chunk_end_ = chunk_start_ + kContextChunkLimit;
  }
  *chunk_start_ = name;
  auto value_handle = chunk_start_ + 1;
  *value_handle = value;
  chunk_start_ += 2;
  return Handle{value_handle};
}

void Context::IterateContext(ObjectVisitor* visitor) {
  IterateContextInternal(Heap::GlobalContext(), visitor);
}

Handle<Object> Context::Search(String* name) {
  for (auto ctx_it = this; ctx_it; ctx_it = ctx_it->parent_) {
    if (ctx_it->chunk_start_) {
      for (auto symbol_it = ctx_it->chunks_.Last();
           symbol_it != ctx_it->chunk_start_; symbol_it += 2) {
        if (*reinterpret_cast<String**>(symbol_it) == name) {
          return Handle{symbol_it + 1};
        }
      }
      for (auto i = 0, len = ctx_it->chunks_.size() - 1; i < len; i++) {
        for (auto symbol_it = ctx_it->chunks_[i],
                  obj_end = ctx_it->chunks_[i] + kContextChunkLimit;
             symbol_it != obj_end; symbol_it += 2) {
          if (*reinterpret_cast<String**>(symbol_it) == name) {
            return Handle{symbol_it + 1};
          }
        }
      }
    }
  }
  return Handle<Object>{};
}

Handle<Object> Context::SearchCurrentContext(String* name) {
  if (chunk_start_) {
    for (auto symbol_it = chunks_.Last(), end = chunk_start_ - 2;
         symbol_it != end; symbol_it += 2) {
      if (*reinterpret_cast<String**>(symbol_it) == name) {
        return Handle{symbol_it + 1};
      }
    }
    for (auto i = chunks_.size() - 2; i >= 0; i--) {
      for (auto symbol = chunks_[i] + kContextChunkLimit - 2, end = chunks_[i];
           symbol != end; symbol -= 2) {
        if (*reinterpret_cast<String**>(symbol) == name) {
          return Handle{symbol + 1};
        }
      }
    }
  }
  return Handle<Object>{};
}

void Context::IterateContextInternal(Context* ctx, ObjectVisitor* visitor) {
  for (auto ctx_it = ctx; ctx_it; ctx_it = ctx_it->next_) {
    if (ctx_it->chunk_start_) {
      for (auto obj_it = ctx_it->chunks_.Last(); obj_it != ctx_it->chunk_start_;
           obj_it++) {
        visitor->Visit(obj_it);
      }
      for (auto i = 0, len = ctx_it->chunks_.size() - 1; i < len; i++) {
        for (auto obj_it = ctx_it->chunks_[i],
                  obj_end = ctx_it->chunks_[i] + kContextChunkLimit;
             obj_it != obj_end; obj_it++) {
          visitor->Visit(obj_it);
        }
      }
    }
  }
}

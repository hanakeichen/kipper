#pragma once

#include "handle.hh"
#include "kipper.hh"
#include "list.hh"

namespace kipper {
namespace internal {

class Object;
class String;

class Context {
 public:
  using Ptr = unique_ptr<Context>;

  explicit Context(Context* parent);

  ~Context();

  Handle<Object> Resolve(String* name);

  Handle<Object> Push(String* name, Object* value);

  Context* parent() { return parent_; }

  Handle<Object> self() const { return self_; }

  void set_self(Handle<Object> self) { self_ = self; }

  static void IterateContext(ObjectVisitor* visitor);

 private:
  Handle<Object> Search(String* name);

  Handle<Object> SearchCurrentContext(String* name);

  static void IterateContextInternal(Context* ctx, ObjectVisitor* visitor);

  Context* parent_;
  Context* next_;
  List<Object**> chunks_;
  Object** chunk_start_;
  Object** chunk_end_;
  Handle<Object> self_;
};

}  // namespace internal
}  // namespace kipper
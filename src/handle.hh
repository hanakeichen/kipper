#pragma once

#include "list.hh"

namespace kipper {
namespace internal {

#ifndef NDEBUG
#define CHECK_TYPE(From, To) \
  do {                       \
    From* from{nullptr};     \
    To* to = from;           \
    assert(from == to);      \
  } while (false)
#else
#define CHECK_TYPE(T, S) ((void*)0)
#endif

class ObjectVisitor;

struct HandleArea {
  void** handle;
  void** end;
  int chunks;
};

class HandleScope {
 public:
  HandleScope() : prev_{current_} { Enter(); }

  ~HandleScope() { Exit(); }

  static void** MakeHandle(void* value);

  static void IterateHandles(ObjectVisitor* visitor);

 private:
  using HandleList = List<void**>;

  void Enter() { current_.chunks = 0; }

  void Exit();

  const HandleArea prev_;
  static HandleArea current_;
  static HandleList handles_;
};

template <class T>
class Handle {
 public:
  explicit Handle() : location_{nullptr} {}

  explicit Handle(T* value)
      : location_{reinterpret_cast<T**>(HandleScope::MakeHandle(value))} {}

  explicit Handle(T** location) : location_{location} {}

  template <class S>
  Handle(Handle<S>& that) : location_{reinterpret_cast<T**>(that.location())} {
    CHECK_TYPE(S, T);
  }

  template <class S>
  Handle(Handle<S>&& that) : location_{reinterpret_cast<T**>(that.location())} {
    CHECK_TYPE(S, T);
  }

  template <class S>
  Handle& operator=(const Handle<S>& that) {
    location_ = reinterpret_cast<T**>(that.location());
    CHECK_TYPE(S, T);
    return *this;
  }

  template <class S>
  Handle& operator=(Handle<S>&& that) {
    location_ = reinterpret_cast<T**>(that.location());
    CHECK_TYPE(S, T);
    return *this;
  }

  T* operator->() const { return Get(); }

  T* operator->() { return Get(); }

  T* operator*() const { return Get(); }

  operator bool() const { return location(); }

  friend bool operator==(const Handle<T>& lhs, const Handle<T>& rhs) {
    return lhs.location() == rhs.location();
  }

  T* Get() const {
    assert(location_ != nullptr);
    return *location_;
  }

  void Clear() { location_ = nullptr; }

  T** location() const { return location_; }

  template <class S>
  static Handle<T> Cast(Handle<S> from) {
    assert(T::Cast(from.Get()) != nullptr);
    return Handle<T>{reinterpret_cast<T**>(from.location())};
  }

 private:
  T** location_;
};

}  // namespace internal
}  // namespace kipper
#pragma once

#include "handle.hh"
#include "value.hh"

namespace kipper {
namespace internal {

struct Completion {
 public:
  enum Type { NORMAL, RETURN, BREAK, CONTINUE };

  Completion(Type type = Type::NORMAL,
             Handle<Object> v = Constant::UndefinedHandle())
      : type{type}, value{v} {}

  Type type;
  Handle<Object> value;
};

}  // namespace internal
}  // namespace kipper
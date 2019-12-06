#pragma once

#include "ast.hh"
#include "interpreter.hh"

namespace kipper {
namespace internal {

class Reference {
 public:
  enum Type { ILLEGAL, NAMED, KEYED, DOTTED };

  Reference(Expression* expr, Execution& exec);

  Reference(MemberAccess* member_access, Execution& exec);

  Handle<Object> SetValue(Handle<Object> value);

  Handle<Object> GetValue() const;

  Handle<Object> GetBase() const { return base_; }

  bool IsPropertyReference() const { return type_ == KEYED || type_ == DOTTED; }

 private:
  Expression* expr_;
  Execution& exec_;
  Handle<Object> base_;
  Handle<Object> key_;
  Type type_{ILLEGAL};
};

}  // namespace internal
}  // namespace kipper
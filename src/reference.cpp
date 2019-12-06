#include "reference.hh"
#include "context.hh"
#include "interpreter.hh"

using namespace kipper::internal;

Reference::Reference(Expression* expr, Execution& exec)
    : expr_{expr}, exec_{exec} {
  if (expr->AsIdentifier()) {
    base_ = exec.context()->Resolve(expr->AsIdentifier()->name.Get());
    type_ = NAMED;
  } else if (auto member_access = expr->AsMemberAccess()) {
    base_ = member_access->target->Evaluate(exec);
    key_ = member_access->member->Evaluate(exec);
    type_ = member_access->type == MemberAccess::KEYED ? KEYED : DOTTED;
  } else {
    throw KSReferenceError{expr_->loc, "reference error"};
  }
}

Reference::Reference(MemberAccess* member_access, Execution& exec)
    : expr_{member_access},
      exec_{exec},
      base_{member_access->target->Evaluate(exec)},
      key_{member_access->member->Evaluate(exec)},
      type_{member_access->type == MemberAccess::KEYED ? KEYED : DOTTED} {}

Handle<Object> Reference::SetValue(Handle<Object> value) {
  assert(type_ != ILLEGAL);
  switch (type_) {
    case NAMED:
      if (base_) {
        *base_.location() = value.Get();
      } else {
        exec_.context()->Push(expr_->AsIdentifier()->name.Get(), value.Get());
      }
      return base_;
    case KEYED:
      if (base_->IsKSArray()) {
        auto index = key_->ToInt32();
        return Handle{Handle<KSArray>::Cast(base_)->Set(index, value.Get())};
      }
      if (base_->IsKSObject()) {
        KSObject::SetProperty(Handle<KSObject>::Cast(base_), key_, value);
        return value;
      }
      break;
    case DOTTED:
      if (base_->IsKSObject()) {
        KSObject::SetProperty(Handle<KSObject>::Cast(base_), key_, value);
        return value;
      }
      break;
    case ILLEGAL:
      break;
  }
  throw KSReferenceError{expr_->loc, "reference error"};
}

Handle<Object> Reference::GetValue() const {
  assert(type_ != ILLEGAL);
  switch (type_) {
    case NAMED:
      return base_;
    case KEYED:
      if (base_->IsKSArray()) {
        return Handle{Handle<KSArray>::Cast(base_)->Get(key_->ToInt32())};
      }
      if (base_->IsKSObject()) {
        return Handle{Handle<KSObject>::Cast(base_)->GetProperty(key_.Get())};
      }
      break;
    case DOTTED:
      if (base_->IsKSObject()) {
        return Handle{Handle<KSObject>::Cast(base_)->GetProperty(key_.Get())};
      }
      break;
    case ILLEGAL:
      break;
  }
  throw KSReferenceError{expr_->loc, "reference error"};
}
#include "ast.hh"
#include <sstream>
#include "context.hh"
#include "heap.hh"
#include "interpreter.hh"
#include "reference.hh"
#include "utils.hh"
#include "value.hh"

using namespace kipper::internal;

#define HANDLE_LOOP_COMPLETION(completion) \
  do {                                     \
    switch ((completion).type) {           \
      case Completion::BREAK:              \
        return Completion();               \
      case Completion::RETURN:             \
        return (completion);               \
      case Completion::CONTINUE:           \
      case Completion::NORMAL:             \
        break;                             \
    }                                      \
  } while (false)

#define THROW_KS_OBJECT_REFERENCE_ERROR(location) \
  throw KSReferenceError { location, "reference error" }

#define THROW_KS_OBJECT_NOT_FUNCTION(location) \
  throw KSNotFunctionError{location, "is not a function"};

#define NODE_ACCEPT(Node) \
  void Node::Accept(NodeVisitor* visitor) { visitor->Visit##Node(this); }
VISIT_NODES(NODE_ACCEPT)
#undef NODE_ACCEPT

Handle<Object> Statement::Evaluate(Execution& /*exec*/) {
  UNREACHABLE();
  return Handle<Object>{};
}

Handle<Object> TranslationUnit::Evaluate(Execution& exec) {
  for (auto& fn_decl : fn_decls) {
    fn_decl->Evaluate(exec);
  }
  for (auto& stmt : stmts) {
    stmt->AsStatement()->Execute(exec);
  }
  return Constant::UndefinedHandle();
}

Completion BlockStatement::Execute(Execution& exec) {
  ExecutionHandler exec_handler{exec};
  for (auto& stmt : stmts) {
    auto completion = stmt->Execute(exec);
    if (completion.type != Completion::NORMAL) {
      return completion;
    }
  }
  return Completion{};
}

Completion IfStatement::Execute(Execution& exec) {
  if (condition->Evaluate(exec)->IsTrue()) {
    return then_stmt->Execute(exec);
  }
  if (else_stmt) {
    return else_stmt->Execute(exec);
  }
  return Completion{};
}

Completion WhileStatement::Execute(Execution& exec) {
  while (condition->Evaluate(exec)->IsTrue()) {
    auto completion = loop_stmt->Execute(exec);
    HANDLE_LOOP_COMPLETION(completion);
  }
  return Completion{};
}

Completion ForStatement::Execute(Execution& exec) {
  if (init) {
    init->Evaluate(exec);
  }
  if (condition) {
    if (update) {
      while (condition->Evaluate(exec)->IsTrue()) {
        auto completion = loop_stmt->Execute(exec);
        HANDLE_LOOP_COMPLETION(completion);
        update->Evaluate(exec);
      }
    } else {
      while (condition->Evaluate(exec)->IsTrue()) {
        auto completion = loop_stmt->Execute(exec);
        HANDLE_LOOP_COMPLETION(completion);
      }
    }
  } else {
    if (update) {
      for (;;) {
        auto completion = loop_stmt->Execute(exec);
        HANDLE_LOOP_COMPLETION(completion);
        update->Evaluate(exec);
      }
    } else {
      for (;;) {
        auto completion = loop_stmt->Execute(exec);
        HANDLE_LOOP_COMPLETION(completion);
      }
    }
  }
  return Completion{};
}

Completion ReturnStatement::Execute(Execution& exec) {
  return value ? Completion{Completion::RETURN, value->Evaluate(exec)}
               : Completion{Completion::RETURN};
}

Completion BreakStatement::Execute(Execution& /* exec */) {
  return Completion{Completion::BREAK};
}

Completion ContinueStatement::Execute(Execution& /* exec */) {
  return Completion{Completion::CONTINUE};
}

Completion ExpressionStatement::Execute(Execution& exec) {
  expr->Evaluate(exec);
  return Completion{};
}

Handle<Object> Assignment::Evaluate(Execution& exec) {
  Reference ref{target.get(), exec};
  auto val = value->Evaluate(exec);
  switch (op) {
    case Token::ASSIGN:
      return ref.SetValue(val);
    case Token::ADD_ASSIGN:
      return ref.SetValue(Interpreter::Add(ref.GetValue(), val));
    case Token::SUB_ASSIGN:
      return ref.SetValue(Interpreter::Sub(ref.GetValue(), val));
    case Token::MUL_ASSIGN:
      return ref.SetValue(Interpreter::Mult(ref.GetValue(), val));
    case Token::DIV_ASSIGN:
      return ref.SetValue(Interpreter::Div(ref.GetValue(), val));
    case Token::MOD_ASSIGN:
      return ref.SetValue(Interpreter::Mod(ref.GetValue(), val));
    default:
      UNREACHABLE();
      return Handle<Object>{};
  }
}

Handle<Object> ConditionalExpression::Evaluate(Execution& exec) {
  return condition->Evaluate(exec)->ToBoolean()->IsTrue()
             ? then_expr->Evaluate(exec)
             : else_expr->Evaluate(exec);
}

Handle<Object> BinaryExpression::Evaluate(Execution& exec) {
  auto left_value = left->Evaluate(exec);
  auto right_value = right->Evaluate(exec);
  switch (op) {
    case Token::PLUS:
      return Interpreter::Add(left_value, right_value);
    case Token::SUB:
      return Interpreter::Sub(left_value, right_value);
    case Token::MUL:
      return Interpreter::Mult(left_value, right_value);
    case Token::DIV:
      return Interpreter::Div(left_value, right_value);
    case Token::MOD:
      return Interpreter::Mod(left_value, right_value);
    case Token::EQ:
      return Constant::BooleanHandle(left_value->Equals(right_value));
    case Token::NE:
      return Constant::BooleanHandle(!left_value->Equals(right_value));
    case Token::LOGIC_OR:
      return Constant::BooleanHandle(left_value->ToBoolean()->IsTrue() ||
                                     right_value->ToBoolean()->IsTrue());
    case Token::LOGIC_AND:
      return Constant::BooleanHandle(left_value->ToBoolean()->IsTrue() &&
                                     right_value->ToBoolean()->IsTrue());
    case Token::LT:
      return Constant::BooleanHandle(left_value->ToDouble() <
                                     right_value->ToDouble());
    case Token::GT:
      return Constant::BooleanHandle(left_value->ToDouble() >
                                     right_value->ToDouble());
    case Token::LTE:
      return Constant::BooleanHandle(left_value->ToDouble() <=
                                     right_value->ToDouble());
    case Token::GTE:
      return Constant::BooleanHandle(left_value->ToDouble() >=
                                     right_value->ToDouble());
    default:
      break;
  }
  assert(Token::IsBinaryOperator(op));
  UNREACHABLE();
  return Handle<Object>{};
}

Handle<Object> UnaryExpression::Evaluate(Execution& exec) {
  switch (op) {
    case Token::PLUS:
      return Handle{target->Evaluate(exec)->ToNumber()};
    case Token::SUB:
      return Handle{Double::Make(-target->Evaluate(exec)->ToDouble())};
    case Token::NOT:
      return Constant::BooleanHandle(!target->Evaluate(exec)->IsTrue());
    case Token::INC: {
      Reference ref{target.get(), exec};
      auto value = ref.GetValue();
      if (value->IsInt32()) {
        return ref.SetValue(
            Handle{Int32::Make(Handle<Int32>::Cast(value)->Value() + 1)});
      }
      if (value->IsHeapNumber()) {
        return ref.SetValue(Handle{
            HeapNumber::New(Handle<HeapNumber>::Cast(value)->Value() + 1)});
      }
      return ref.SetValue(Handle{Double::Make(value->ToDouble() + 1)});
    }
    case Token::DEC: {
      Reference ref{target.get(), exec};
      auto value = ref.GetValue();
      if (value->IsInt32()) {
        return ref.SetValue(
            Handle{Int32::Make(Handle<Int32>::Cast(value)->Value() - 1)});
      }
      if (value->IsHeapNumber()) {
        return ref.SetValue(Handle{
            HeapNumber::New(Handle<HeapNumber>::Cast(value)->Value() - 1)});
      }
      return ref.SetValue(Handle{Double::Make(value->ToDouble() - 1)});
    }
    default:
      UNREACHABLE();
      return Handle<Object>{};
  }
}

Handle<Object> PostfixExpression::Evaluate(Execution& exec) {
  Reference ref{target.get(), exec};
  if (op == Token::INC) {
    auto value = ref.GetValue();
    if (value->IsInt32()) {
      ref.SetValue(
          Handle{Int32::Make(Handle<Int32>::Cast(value)->Value() + 1)});
    } else if (value->IsHeapNumber()) {
      ref.SetValue(Handle{
          HeapNumber::New(Handle<HeapNumber>::Cast(value)->Value() + 1)});
    } else {
      ref.SetValue(Handle{Double::Make(value->ToDouble() + 1)});
    }
    return value;
  }
  auto value = ref.GetValue();
  if (value->IsInt32()) {
    ref.SetValue(Handle{Int32::Make(Handle<Int32>::Cast(value)->Value() - 1)});
  } else if (value->IsHeapNumber()) {
    ref.SetValue(
        Handle{HeapNumber::New(Handle<HeapNumber>::Cast(value)->Value() - 1)});
  } else {
    ref.SetValue(Handle{Double::Make(value->ToDouble() - 1)});
  }
  return value;
}

Handle<Object> MemberAccess::Evaluate(Execution& exec) {
  return Reference{this, exec}.GetValue();
}

Handle<Object> IntLiteral::Evaluate(Execution& /* exec */) { return value_; }

Handle<Object> DoubleLiteral::Evaluate(Execution& /* exec */) { return value_; }

Handle<Object> StringLiteral::Evaluate(Execution& /* exec */) { return value_; }

Handle<Object> BooleanLiteral::Evaluate(Execution& /* exec */) {
  return value_;
}

Handle<Object> ArrayLiteral::Evaluate(Execution& exec) {
  auto size = static_cast<int32_t>(elements.size());
  auto result = Handle{KSArray::New(size)};
  for (int i = 0; i < size; i++) {
    auto element = elements[i]->Evaluate(exec);
    result->Set(i, element.Get());
  }
  return result;
}

Handle<Object> ObjectLiteral::Evaluate(Execution& exec) {
  auto object = Handle{KSObject::New(static_cast<int32_t>(properties.size()))};
  for (auto& prop : properties) {
    auto key = prop->name->Evaluate(exec);
    auto value = prop->value->Evaluate(exec);
    KSObject::SetProperty(object, key, value);
  }
  return object;
}

Handle<Object> UndefinedLiteral::Evaluate(Execution& /*exec*/) {
  return Constant::UndefinedHandle();
}

Handle<Object> Identifier::Evaluate(Execution& exec) {
  auto result = exec.context()->Resolve(name.Get());
  return result ? result : Constant::UndefinedHandle();
}

Handle<Object> IdentifierName::Evaluate(Execution& /*exec*/) { return name; }

Handle<Object> FunctionCall::Evaluate(Execution& exec) {
  Reference ref{target.get(), exec};
  if (auto result = ref.GetValue(); result->IsFunction()) {
    Handle<Object> self;
    if (ref.IsPropertyReference()) {
      self = ref.GetBase();
    }
    auto args_size = static_cast<int32_t>(args.size());
    auto arguments = Handle{KSArray::New(args_size, TENURED)};
    for (int i = 0; i < args_size; i++) {
      auto arg = args[i]->Evaluate(exec);
      arguments->Set(i, arg.Get());
    }
    if (Function::Cast(result.Get())->Name()->Value() == "Assert") {
      std::stringstream loc;
      loc << args[0]->loc;
      KSArray::Push(arguments, Handle{String::New(loc.str(), TENURED)});
    }
    try {
      return exec.interpreter()->Call(self, result, arguments, exec.context());
    } catch (const KSNotFunctionError&) {
      THROW_KS_OBJECT_NOT_FUNCTION(target->loc);
    }
  }
  THROW_KS_OBJECT_NOT_FUNCTION(target->loc);
}

Handle<Object> FunctionDecl::Evaluate(Execution& exec) {
  auto params_size = static_cast<int32_t>(params.size());
  auto params_array = Handle{Array::New(params_size, TENURED)};
  for (int i = 0; i < params_size; i++) {
    auto param = params[i]->Evaluate(exec);
    params_array->Set(i, param.Get());
  }
  auto fn = Function::New(name.Get(), params_array.Get(), &body, TENURED);
  exec.context()->Push(name.Get(), fn);
  return Constant::UndefinedHandle();
}
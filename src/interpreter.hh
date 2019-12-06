#pragma once

#include <cmath>
#include <memory>
#include <unordered_map>
#include "context.hh"
#include "handle.hh"
#include "heap.hh"

namespace kipper {
namespace internal {

class Interpreter;
class Object;
class String;
class Location;
struct Node;

class Execution {
 public:
  Execution(Interpreter* interpreter, Context* context)
      : interpreter_{interpreter}, context_{context} {}

  Context* context() const { return context_; }

  Interpreter* interpreter() const { return interpreter_; }

 private:
  friend class ExecutionHandler;
  friend class Interpreter;

  Interpreter* interpreter_;
  Context* context_;
};

class Interpreter {
 public:
  Handle<Object> Evaluate(std::string_view code, std::string_view filename,
                          Context* context);

  Handle<Object> Evaluate(Handle<String> code, std::string_view filename,
                          Context* context);

  Handle<Object> Evaluate(Node* ast, Context* context);

  Handle<Object> Call(Handle<Object> self, Handle<Object> obj,
                      Handle<KSArray> args, Context* context);

  static Handle<Object> Add(Handle<Object> left, Handle<Object> right) {
    if (left->IsString() || right->IsString()) {
      return Handle{left->ToString()->Concat(right->ToString())};
    }
    return Handle{Double::MakeFit(left->ToDouble() + right->ToDouble())};
  }

  static Handle<Object> Sub(Handle<Object> left, Handle<Object> right) {
    return Handle{Double::MakeFit(left->ToDouble() - right->ToDouble())};
  }

  static Handle<Object> Mult(Handle<Object> left, Handle<Object> right) {
    return Handle{Double::MakeFit(left->ToDouble() * right->ToDouble())};
  }

  static Handle<Object> Div(Handle<Object> left, Handle<Object> right) {
    return Handle{Double::MakeFit(left->ToDouble() / right->ToDouble())};
  }

  static Handle<Object> Mod(Handle<Object> left, Handle<Object> right) {
    return Handle{Double::MakeFit(fmod(left->ToDouble(), right->ToDouble()))};
  }
};

class ExecutionHandler {
 public:
  explicit ExecutionHandler(Execution& exec)
      : exec_{exec}, current_context_{exec.context_} {
    exec_.context_ = &current_context_;
  }

  ~ExecutionHandler() { exec_.context_ = current_context_.parent(); }

  DISABLE_DEFAULT_OP(ExecutionHandler)
 private:
  Execution& exec_;
  Context current_context_;
  HandleScope handle_scope_;
};

class KSReferenceError : public KSError {
 public:
  KSReferenceError(const Location& loc, const char* msg) : KSError{loc, msg} {}
};

class KSNotFunctionError : public KSError {
 public:
  KSNotFunctionError(const Location& loc, const std::string& msg)
      : KSError{loc, msg} {}

  KSNotFunctionError(const Location& loc, const char* msg)
      : KSError{loc, msg} {}
};

}  // namespace internal
}  // namespace kipper
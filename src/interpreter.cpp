#include "interpreter.hh"
#include <memory>
#include "ast.hh"
#include "compiler.hh"
#include "context.hh"
#include "kipper.hh"
#include "message.hh"
#include "value.hh"

using namespace kipper::internal;

Handle<Object> Interpreter::Evaluate(std::string_view code,
                                     std::string_view filename,
                                     Context* context) {
  return Evaluate(Compiler::Compile(code, filename).get(), context);
}

Handle<Object> Interpreter::Evaluate(Handle<String> code,
                                     std::string_view filename,
                                     Context* context) {
  return Evaluate(code->Value(), filename, context);
}

Handle<Object> Interpreter::Evaluate(Node* ast, Context* context) {
  if (ast == nullptr) {
    return Constant::UndefinedHandle();
  }
  Execution exec{this, context};
  ExecutionHandler exec_handler{exec};
  return ast->Evaluate(exec);
}

Handle<Object> Interpreter::Call(Handle<Object> self, Handle<Object> obj,
                                 Handle<KSArray> args, Context* context) {
  Execution exec{this, context};
  if (obj && obj->IsFunction()) {
    auto fn_decl = Function::Cast(*obj);
    auto params = fn_decl->Params();
    {
      Handle<Object> return_val{static_cast<Object*>(nullptr)};
      ExecutionHandler exec_handler{exec};
      exec.context()->set_self(self);
      for (int i = 0, params_lenth = params->Length(); i < params_lenth; i++) {
        exec.context()->Push(String::Cast(params->Get(i)), args->Get(i));
      }

      exec.context()->Push(String::NewSymbol("arguments_"), args.Get());
      if (fn_decl->IsFunctionTemplate()) {
        return fn_decl->Body()(args, exec.context());
      }
      for (auto& stmt : *static_cast<FunctionDecl::Body*>(fn_decl->KSBody())) {
        auto completion = stmt->Execute(exec);
        if (completion.type == Completion::RETURN) {
          *return_val.location() = completion.value.Get();
          return return_val;
        }
      }
      return Constant::UndefinedHandle();
    }
  }
  throw KSNotFunctionError{Location{}, "object is not a function"};
}

#include "runtime.hh"
#include <iostream>
#include "context.hh"
#include "heap.hh"
#include "value.hh"

using namespace kipper::internal;

bool Runtime::installed_{false};

void Runtime::InstallNative() {
  InstallNativeProperties();
  InstallNativeFunctions();
}

void Runtime::InstallNativeProperties() {
  if (installed_) {
    return;
  }
  installed_ = true;
  static auto array_push_fn = Handle{Function::New(
      String::NewSymbol("push"), Array::New(0),
      [](Handle<KSArray> args, Context* context) -> Handle<Object> {
        if (context->self()) {
          auto arg = Handle{args->Get(0)};
          KSArray::Push(Handle<KSArray>::Cast(context->self()), arg);
          return Constant::UndefinedHandle();
        }
        return Handle<Object>{};
      })};

  KSObject::AddGetPropertyInterceptor(
      [](KSObject* obj, String* key) -> Handle<Object> {
        if (obj->IsKSArray()) {
          if (key->Value() == "length") {
            return Handle{Int32::Make(KSArray::Cast(obj)->Length())};
          }
          if (key->Value() == "push") {
            return array_push_fn;
          }
        } else if (obj->IsString()) {
          if (key->Value() == "length") {
            return Handle{Int32::Make(String::Cast(obj)->Length())};
          }
        }
        return Handle<Object>{};
      });
}

void Runtime::InstallNativeFunctions() {
  {
    auto fn_name = String::NewSymbol("Print");
    auto print_fn = Function::New(
        fn_name, Array::New(0),
        [](Handle<KSArray> args, Context * /*context*/) -> Handle<Object> {
          for (auto i = 0, len = args->Length(); i < len; i++) {
            std::cout << args->Get(i)->ToString()->Value();
            if (i < len - 1) {
              std::cout << ", ";
            }
          }
          std::cout << "\n";
          return Constant::UndefinedHandle();
        });
    Heap::GlobalContext()->Push(fn_name, print_fn);
  }
}
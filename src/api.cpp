#include <cassert>
#include "ast.hh"
#include "compiler.hh"
#include "context.hh"
#include "handle.hh"
#include "heap.hh"
#include "interpreter.hh"
#include "kipper.hh"
#include "kipper/kipper.hh"
#include "value.hh"

using namespace kipper;

namespace i = kipper::internal;

#ifndef NDEBUG
#define LOG_API(api) printf("call kipper api: %s\n", api)
#else
#define LOG_API(api) ((void)0)
#endif // !NDEBUG

#define CONST_API_CAST(From, To)                                        \
  inline static i::To* ApiCast(const kipper::From* source) {            \
    return reinterpret_cast<i::To*>(const_cast<kipper::From*>(source)); \
  }

#define API_CAST(From, To) \
  inline static To ApiCast(From from) { return reinterpret_cast<To>(from); }

#define EXPORT_HANDLE(From, To)                                                \
  inline static Handle<To> Export##To(i::From** from) {                        \
    return Handle<kipper::To>(reinterpret_cast<kipper::To*>(from));            \
  }                                                                            \
                                                                               \
  inline static Handle<To> Export##To(i::Handle<i::From> from) {               \
    return Handle<kipper::To>(reinterpret_cast<kipper::To*>(from.location())); \
  }

#define IMPORT_HANDLE(From, To)                                          \
  inline static i::Handle<i::To> Import##To(const kipper::From* from) {  \
    return i::Handle<i::To>(                                             \
        reinterpret_cast<i::To**>(const_cast<kipper::From*>(from)));     \
  }                                                                      \
                                                                         \
  inline static i::Handle<i::To> Import##To(Handle<kipper::From> from) { \
    return i::Handle<i::To>(reinterpret_cast<i::To**>(from.Get()));      \
  }

#define CONVERT(from, That)                                                    \
  do {                                                                         \
    if (Is##That()) {                                                          \
      return Handle<That>(                                                     \
          static_cast<kipper::That*>(const_cast<kipper::Value*>(from)));       \
    }                                                                          \
    return Export##That(i::Handle{ImportObject(from)->To##That()}.location()); \
  } while (false)

CONST_API_CAST(Context, Context)
CONST_API_CAST(Script, Node)

API_CAST(i::Context*, Context*)
API_CAST(i::Node*, Script*)
API_CAST(kSFunctionTemplate, i::Function::FunctionTemplate)

EXPORT_HANDLE(Object, Number)
EXPORT_HANDLE(String, String)
EXPORT_HANDLE(Double, Number)
EXPORT_HANDLE(Int32, Number)
EXPORT_HANDLE(HeapNumber, Number)
EXPORT_HANDLE(KSArray, Array)
EXPORT_HANDLE(Object, Value)
EXPORT_HANDLE(Object, Boolean)
EXPORT_HANDLE(Function, Function)
EXPORT_HANDLE(KSObject, Object)

IMPORT_HANDLE(Value, Object)
IMPORT_HANDLE(Array, KSArray)
IMPORT_HANDLE(String, String)
IMPORT_HANDLE(Function, Function)
IMPORT_HANDLE(Object, KSObject)

bool Value::IsNumber() const {
  LOG_API("Value::IsNumber");
  return ImportObject(this)->IsNumber();
}

bool Value::IsBoolean() const {
  LOG_API("Value::IsBoolean");
  return ImportObject(this)->IsBoolean();
}

bool Value::IsString() const {
  LOG_API("Value::IsString");
  return ImportObject(this)->IsString();
}

bool Value::IsArray() const {
  LOG_API("Value::IsArray");
  return ImportObject(this)->IsKSArray();
}

bool Value::IsNull() const {
  LOG_API("Value::IsNull");
  return ImportObject(this)->IsNull();
}

bool Value::IsUndefined() const {
  LOG_API("Value::IsUndefined");
  return ImportObject(this)->IsUndefined();
}

bool Value::IsFunction() const {
  LOG_API("Value::IsKSFunction");
  return ImportObject(this)->IsFunction();
}

bool Value::IsObject() const {
  LOG_API("Value::IsKSObject");
  return ImportObject(this)->IsKSObject();
}

Handle<Number> Value::ToNumber() const {
  LOG_API("Value::ToNumber");
  CONVERT(this, Number);
}

Handle<Boolean> Value::ToBoolean() const {
  LOG_API("Value::ToBoolean");
  CONVERT(this, Boolean);
}

Handle<String> Value::ToString() const {
  LOG_API("Value::ToString");
  CONVERT(this, String);
}

bool Value::Equals(Handle<Value> that) const {
  LOG_API("Value::Equals");
  return ImportObject(this)->Equals(ImportObject(that));
}

double Number::Double() const {
  LOG_API("Number::Double");
  assert(IsNumber());
  return ImportObject(this)->ToDouble();
}

int32_t Number::Int32() const {
  LOG_API("Number::Int32");
  assert(IsNumber());
  return ImportObject(this)->ToInt32();
}

int64_t Number::Int64() const {
  LOG_API("Number::Int64");
  assert(IsNumber());
  return ImportObject(this)->ToInt64();
}

Handle<Number> Number::New(double value) {
  LOG_API("Number::New(double)");
  return ExportNumber(i::Handle{i::Double::Make(value)});
}

Handle<Number> Number::New(int32_t value) {
  LOG_API("Number::New(int32_t)");
  return ExportNumber(i::Handle{i::Int32::Make(value)});
}

Handle<Number> Number::New(int64_t value) {
  LOG_API("Number::New(int64_t)");
  return ExportNumber(i::Handle{i::HeapNumber::New(value)});
}

Number* Number::Cast(Value* obj) {
  LOG_API("Number::Cast");
  return static_cast<Number*>(obj);
}

Handle<Boolean> Boolean::New(bool value) {
  LOG_API("Boolean::New");
  return ExportBoolean(i::Constant::BooleanHandle(value));
}

bool Boolean::Value() const {
  LOG_API("Boolean::Value");
  assert(IsBoolean());
  return ImportObject(this)->IsTrue();
}

int String::Length() const {
  LOG_API("String::Length");
  assert(IsString());
  return ImportString(this)->Length();
}

Handle<String> String::Concat(Handle<String> that) const {
  LOG_API("String::Concat");
  assert(IsString());
  assert(that->IsString());
  return ExportString(
      i::Handle{ImportString(this)->Concat(ImportString(that).Get())});
}

std::string_view String::StringView() const {
  LOG_API("String::StringView");
  assert(IsString());
  return ImportString(this)->Value();
}

Handle<String> String::New(std::string_view value) {
  LOG_API("String::New");
  return ExportString(i::Handle{i::String::New(value)});
}

String* String::Cast(Value* obj) {
  LOG_API("String::Cast");
  return static_cast<String*>(obj);
}

Handle<Value> Object::GetProperty(Handle<Value> key) const {
  LOG_API("KSObject::GetProperty");
  return ExportValue(
      i::Handle{ImportKSObject(this)->GetProperty(ImportObject(key).Get())});
}

void Object::SetProperty(Handle<Value> key, Handle<Value> value) {
  LOG_API("KSObject::SetProperty");
  i::KSArray::SetProperty(ImportKSObject(this), ImportObject(key),
                          ImportObject(value));
}

Handle<Object> Object::New(int32_t length) {
  LOG_API("KSObject::New");
  return ExportObject(i::Handle{i::KSObject::New(length)});
}

Object* Object::Cast(Value* obj) {
  LOG_API("KSObject::Cast");
  return static_cast<Object*>(obj);
}

int32_t Array::Length() const {
  LOG_API("Array::Length");
  return ImportKSArray(this)->Length();
}

void Array::Push(Handle<Value> value) {
  LOG_API("Array::Push");
  i::KSArray::Push(ImportKSArray(this), ImportObject(value));
}

void Array::Set(int32_t index, Handle<Value> value) {
  LOG_API("Array::Set");
  ImportKSArray(this)->Set(index, ImportObject(value).Get());
}

Handle<Value> Array::Index(int32_t index) const {
  LOG_API("Array::Index");
  return ExportValue(i::Handle{ImportKSArray(this)->Get(index)});
}

Handle<Array> Array::New(int32_t length) {
  LOG_API("Array::New");
  return ExportArray(i::Handle{i::KSArray::New(length)});
}

Array* Array::Cast(Value* obj) {
  LOG_API("Array::Cast");
  return static_cast<Array*>(obj);
}

Handle<Value> Function::Call(Handle<Object> self, Handle<Array> args,
                             Context* context) const {
  LOG_API("KSFunction::Call");
  return ExportValue(ImportFunction(this)->Call(
      ImportObject(self), ImportKSArray(args), ApiCast(context)));
}

Handle<Function> Function::New(std::string_view name, int argc,
                               std::string_view* params,
                               kSFunctionTemplate fn_template) {
  LOG_API("KSFunction::New");
  auto fn_params = i::Array::New(argc);
  for (int i = 0; i < argc; i++) {
    auto param = i::String::NewSymbol(params[i]);
    fn_params->Set(i, param);
  }
  return ExportFunction(i::Handle{i::Function::New(
      i::String::NewSymbol(name), fn_params, ApiCast(fn_template))});
}

Function* Function::Cast(Value* obj) {
  LOG_API("KSFunction::New");
  return static_cast<Function*>(obj);
}

Handle<Value> kipper::Undefined() {
  LOG_API("Undefined()");
  return ExportValue(i::Constant::UndefinedHandle());
}

Handle<Value> kipper::Null() {
  LOG_API("Null()");
  return ExportValue(i::Constant::NullHandle());
}

Handle<Value> Script::Run(Context* context) const {
  LOG_API("Script::Run");
  return ExportValue(
      i::Kipper::interpreter()->Evaluate(ApiCast(this), ApiCast(context)));
}

Script::Ptr Script::Compile(std::string_view code, std::string_view filename) {
  LOG_API("Script::Compile");
  return Script::Ptr(ApiCast(i::Compiler::Compile(code, filename).release()),
                     [](Script* script) { delete ApiCast(script); });
}

void Context::Push(std::string_view name, Handle<Value> value) {
  LOG_API("Context::Push");
  ApiCast(this)->Push(i::String::NewSymbol(name), ImportObject(value).Get());
}

Handle<Value> Context::Resolve(std::string_view name) const {
  LOG_API("Context::Resolve");
  return ExportValue(ApiCast(this)->Resolve(i::String::NewSymbol(name)));
}

void Kipper::Initialize() {
  LOG_API("Kipper::Initialize");
  i::Kipper::Initialize();
}

void Kipper::Configure(const KipperConfig& config) {
  i::Heap::Configure(config.heap_size, config.tenure_threshold);
}

Context* Kipper::GlobalContext() {
  LOG_API("Kipper::GlobalContext");
  return ApiCast(i::Heap::GlobalContext());
}

Context::Ptr Kipper::CreateContext(Context* parent) {
  LOG_API("Kipper::CreateContext");
  return Context::Ptr(ApiCast(new i::Context{ApiCast(parent)}),
                      [](Context* context) { delete ApiCast(context); });
}
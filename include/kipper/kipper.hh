#pragma once

#include <memory>
#include <string_view>

namespace kipper {

class Boolean;
class Context;
class Number;
class String;

template <class T>
class Handle {
 public:
  explicit Handle(T* value) : value_{value} {}

  template <class S>
  Handle(Handle<S>& that) : value_{static_cast<T*>(that.Get())} {}

  template <class S>
  Handle(Handle<S>&& that) : value_{static_cast<T*>(that.Get())} {
    that.Clear();
  }

  T* Get() const { return value_; }

  bool IsEmpty() const { return value_ == nullptr; }

  void Clear() { value_ = nullptr; }

  T* operator->() const { return value_; }

  T* operator*() const { return value_; }

  template <class S>
  bool operator==(Handle<S> that) const {
    void** a = reinterpret_cast<void**>(**this);
    void** b = reinterpret_cast<void**>(*that);
    return a == b || *a == *b;
  }

  static Handle<T> Empty() { return Handle{nullptr}; }

  template <class S>
  static Handle<T> Cast(Handle<S> from) {
    if (from.IsEmpty()) {
      return Empty();
    }
    return Handle<T>{T::Cast(from.Get())};
  }

 private:
  T* value_;
};

class Value {
 public:
  bool IsNumber() const;
  bool IsBoolean() const;
  bool IsString() const;
  bool IsArray() const;
  bool IsNull() const;
  bool IsUndefined() const;
  bool IsFunction() const;
  bool IsObject() const;

  Handle<Number> ToNumber() const;
  Handle<Boolean> ToBoolean() const;
  Handle<String> ToString() const;

  bool Equals(Handle<Value> that) const;

  static Value* Cast(Value* obj);

 private:
  Value();
};

class Number : public Value {
 public:
  double Double() const;

  int32_t Int32() const;

  int64_t Int64() const;

  static Handle<Number> New(double value);
  static Handle<Number> New(int32_t value);
  static Handle<Number> New(int64_t value);

  static Number* Cast(Value* obj);

 private:
  Number();
};

class Boolean : public Value {
 public:
  bool Value() const;

  static Handle<Boolean> New(bool value);

 private:
  Boolean();
};

class Object : public Value {
 public:
  Handle<Value> GetProperty(Handle<Value> key) const;

  void SetProperty(Handle<Value> key, Handle<Value> value);

  static Handle<Object> New(int32_t length);

  static Object* Cast(Value* obj);

 private:
  Object();
};

class String : public Object {
 public:
  int32_t Length() const;

  Handle<String> Concat(Handle<String> that) const;

  std::string_view StringView() const;

  static Handle<String> New(std::string_view value);

  static String* Cast(Value* obj);

 private:
  String();
};

class Array : public Object {
 public:
  int32_t Length() const;

  void Push(Handle<Value> value);

  void Set(int32_t index, Handle<Value> value);

  Handle<Value> Index(int32_t index) const;

  static Handle<Array> New(int32_t length);

  static Array* Cast(Value* obj);

 private:
  Array();
};

using kSFunctionTemplate = Handle<Value> (*)(Handle<Array> args,
                                             Context* context);

class Function : public Value {
 public:
  Handle<Value> Call(Handle<Object> self, Handle<Array> args,
                     Context* context) const;

  static Handle<Function> New(std::string_view name, int argc,
                              std::string_view* params,
                              kSFunctionTemplate fn_template);

  template <int ArgsN>
  static Handle<Function> New(std::string_view name,
                              std::string_view (&)[ArgsN],
                              kSFunctionTemplate fn_template);

  static Function* Cast(Value* obj);
};

Handle<Value> Undefined();
Handle<Value> Null();

class Script {
 public:
  using Ptr = std::unique_ptr<Script, void (*)(Script*)>;

  Handle<Value> Run(Context* context) const;

  static Script::Ptr Compile(std::string_view code, std::string_view filename);

 private:
  Script();
};

class Context {
 public:
  using Ptr = std::unique_ptr<Context, void (*)(Context*)>;

  void Push(std::string_view name, Handle<Value> value);

  Handle<Value> Resolve(std::string_view name) const;

 private:
  Context();
};

struct KipperConfig {
  size_t heap_size;
  uint8_t tenure_threshold;
};

class Kipper {
 public:
  static void Configure(const KipperConfig& config);

  static void Initialize();

  static Context* GlobalContext();

  static Context::Ptr CreateContext(Context* parent);
};

template <int ArgsN>
inline Handle<Function> kipper::Function::New(std::string_view name,
                                              std::string_view (&params)[ArgsN],
                                              kSFunctionTemplate fn_template) {
  return Function::New(name, ArgsN, params, fn_template);
}

}  // namespace kipper

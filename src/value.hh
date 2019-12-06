#pragma once

#include <cstdint>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>
#include "handle.hh"
#include "kipper.hh"
#include "utils.hh"

namespace kipper {
namespace internal {

class Array;
class Context;
class HashTable;
class HeapObject;
class MetaMetadata;
class Object;
class ObjectVisitor;
class String;
class KSObject;

static_assert(kPointerSize == 8);

constexpr int kCanonicalBits = 48;
constexpr int kMetadataEncodedBits = 8;
constexpr int kAgeBits = 8;
constexpr uint64_t kObjectMask = (1ULL << kCanonicalBits) - 1;
constexpr int kMaxMetadataCount = (1 << kMetadataEncodedBits) - 1;

using KSObjectGetPropertyInterceptor = Handle<Object> (*)(KSObject* obj,
                                                          String* key);

enum AllocationPolicy { NOT_TENURED, TENURED };

enum HeapObjectType { KSOBJECT, STRING, ARRAY, KSARRAY, HEAP_NUMBER, FUNCTION };

class Object {
 public:
  bool IsNumber();
  bool IsDouble();
  bool IsInt32();

  bool IsBoolean();
  bool IsTrue();
  bool IsNull();
  bool IsUndefined();
  bool IsFunction();

  bool IsHeapObject();
  bool IsKSObject();
  bool IsString();
  bool IsArray();
  bool IsKSArray();
  bool IsHeapNumber();

  Object* ToBoolean();
  Object* ToNumber();
  String* ToString();

  double ToDouble();
  int32_t ToInt32();
  int64_t ToInt64();
  std::string ToStdString();

  bool Equals(Object* that);

  bool Equals(Handle<Object> that) { return Equals(*that); }

  static Object* Cast(Object*);

  static constexpr int kSize = 0;
};

class Double : public Object {
 public:
  double Value();

  static Double* Make(double value);

  static Object* MakeFit(double value);

  static Double* NaN();

  static Double* Cast(Object* obj);

  static constexpr uint64_t kDoubleLimit = 0xfff8000000000000;
  static constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
  static constexpr int kSize = Object::kSize + sizeof(double);
  static_assert(kSize == 8);

  DISABLE_DEFAULT_OP(Double)
};

class Int32 : public Object {
 public:
  int32_t Value();

  static Int32* Make(int32_t value);

  static bool Fit(double value) {
    return value >= static_cast<double>(kMinInt32) &&
           value <= static_cast<double>(kMaxInt32);
  }

  static Int32* Cast(Object* obj);

  static constexpr uint64_t kInt32Tag = 0xfff9000000000000;
  static constexpr int32_t kMaxInt32 = 0x7fffffff;
  static constexpr int32_t kMinInt32 = ~kMaxInt32;
  static constexpr int kSize = Object::kSize + sizeof(int32_t);
  static_assert(kSize == 4);

  DISABLE_DEFAULT_OP(Int32)
};

class Constant : public Object {
 public:
  static Object* Boolean(bool value);
  static Object* Null();
  static Object* Undefined();

  static Handle<Object> BooleanHandle(bool value);
  static Handle<Object> NullHandle();
  static Handle<Object> UndefinedHandle();

  static constexpr uint64_t kConstantTag = 0xfffa000000000000;
  static constexpr uint64_t kBoolTrue = kConstantTag;
  static constexpr uint64_t kBoolFalse = kBoolTrue + 1;
  static constexpr uint64_t kNull = kBoolFalse + 1;
  static constexpr uint64_t kUndefined = kNull + 1;

  DISABLE_DEFAULT_OP(Constant)
 private:
  static Object* true_value_;
  static Object* false_value_;
  static Object* null_value_;
  static Object* undefined_value_;
};

/// 63-48
/// |xxxxxxxx|xxxxxxxx|
/// |---age--|--type--|
///
/// 47-3
/// |ppppppppppppppppppppppppppppppppppppppppppppp|
/// |-----------Forwarding Pointer----------------|
///
/// 0-2
/// |x|-----x----|---x--|
/// |-|remembered|marked|
class Metadata {
 public:
  enum : uint64_t {
    MarkedTagMask = 1,
    RememberedTagMask = 1 << 1,
    ForwardingMask = ((1ULL << kCanonicalBits) - 1) - (kPointerSize - 1),
    TypeBitsOffset = kCanonicalBits,
    AgeBitsOffset = TypeBitsOffset + kMetadataEncodedBits,
    TypeMask = ((1ULL << kMetadataEncodedBits) - 1) << TypeBitsOffset,
    AgeMask = ((1ULL << kAgeBits) - 1) << AgeBitsOffset,
  };

  Metadata(HeapObject* obj);

  uint8_t Age() const;

  bool IsForwarding() const;

  bool IsRemembered() const;

  bool IsMarked() const;

  HeapObject* Forwarding() const;

  HeapObjectType Type() const;

  void IncrementAge();

  void Forwarding(Address addr);

  void Remember();

  void Mark();

  void ResetForwarding();

  void ResetRemembered();

  void ResetMarked();

  void set_type(HeapObjectType type);

  uint64_t EncodedMetadata() const;

 private:
  uint64_t metadata_;
};

class HeapObject : public Object {
 public:
  Address address();

  Metadata metadata();

  void set_metadata(Metadata metadata);

  int Size();

  void IterateBody(ObjectVisitor* visitor);

  static HeapObject* Make(Address addr);

  static HeapObject* Cast(Object* obj);

  static constexpr uint64_t kHeapObjectTag = 0xfffc000000000000;
  static constexpr int kMetadataOffset = Object::kSize;
  static constexpr int kHeaderSize = kMetadataOffset + kPointerSize;

  DISABLE_DEFAULT_OP(HeapObject)
 private:
  using Object::kSize;
};

class KSObject : public HeapObject {
 public:
  Object* GetProperty(Object* key);

  HashTable* Elements();

  void SetElements(HashTable* elements);

  void IterateKSObjectBody(ObjectVisitor* visitor);

  static void SetProperty(Handle<KSObject> self, Handle<Object> key,
                          Handle<Object> value);

  static KSObject* New(int32_t elements_size,
                       AllocationPolicy policy = NOT_TENURED);

  static KSObject* Cast(Object* obj);

  static void AddGetPropertyInterceptor(
      KSObjectGetPropertyInterceptor interceptor);

  static constexpr int kElementsOffset = HeapObject::kHeaderSize;
  static constexpr int kSize = kElementsOffset + kPointerSize;

  DISABLE_DEFAULT_OP(KSObject)
 private:
  using HeapObject::kHeaderSize;

  Object* SetProperty(Object* key, Object* value);

  static std::vector<KSObjectGetPropertyInterceptor> get_property_interceptors_;
};

class String : public KSObject {
 public:
  int32_t Length();

  String* Concat(String* that);

  std::string_view Value();

  void Length(int32_t length);

  void Content(std::string_view value);

  int Hash();

  static int Hash(std::string_view value);

  static int EnsureSize(int32_t length) { return Align(kBytesOffset + length); }

  static String* New(std::string_view value,
                     AllocationPolicy policy = NOT_TENURED);

  static String* NewSymbol(std::string_view value);

  static String* Cast(Object* obj);

  static constexpr int kLengthOffset = KSObject::kSize;
  static constexpr int kBytesOffset = kLengthOffset + Int32::kSize;

  DISABLE_DEFAULT_OP(String)
};

class Array : public HeapObject {
 public:
  int32_t Length();

  void SetLength(int32_t length);

  Object* Get(int32_t index);

  Object* Set(int32_t index, Object* value);

  void Copy(Array* that);

  void IterateArrayBody(ObjectVisitor* visitor);

  static int EnsureSize(int length) {
    return Align(kElementsOffset + kPointerSize * length);
  }

  static Array* New(int32_t length, AllocationPolicy policy = NOT_TENURED);

  static Array* Cast(Object* obj);

  static constexpr int kLengthOffset = HeapObject::kHeaderSize;
  static constexpr int kElementsOffset = kLengthOffset + Int32::kSize;

  DISABLE_DEFAULT_OP(Array)

 protected:
  Object** GetHandle(int32_t index);
};

class HashTable : public Array {
 public:
  HashTable* Insert(String* key, Object* value);

  Object* Search(String* key);

  bool Delete(String* key);

  int ElementsSize() { return Get(kElementsSizeIndex)->ToInt32(); }

  void SetElementsSize(int elements_size) {
    assert(elements_size >= 0);
    Set(kElementsSizeIndex, Int32::Make(elements_size));
  }

  int Capacity() { return Get(kCapacityIndex)->ToInt32(); }

  void SetCapacity(int capacity) {
    assert(capacity > ElementsSize());
    Set(kCapacityIndex, Int32::Make(capacity));
  }

  void IterateHashTableBody(ObjectVisitor* visitor);

  static int EnsureSize(int capacity) { return EntryToIndex(capacity); }

  static HashTable* Cast(Object* obj);

  static constexpr int kElementsSizeIndex = 0;
  static constexpr int kCapacityIndex = 1;
  static constexpr int kElementsOffsetIndex = 2;

 private:
  HashTable* AddElement(int count);

  void ElementAdded() { SetElementsSize(ElementsSize() + 1); }

  void ElementDeleted() { SetElementsSize(ElementsSize() - 1); }

  int FindInsertionIndex(int hash);

  int FindEntry(String* key, int hash);

  void SetEntry(int index, Object* key, Object* value);

  Object* EntryKeyAt(int index) { return Get(index); }

  static constexpr int Location(int hash, int index, int capacity) {
    return (hash + ((index + index * index) >> 1)) & (capacity - 1);
  }

  static constexpr int EntryToIndex(int entry) {
    assert(entry >= 0);
    return kElementsOffsetIndex + entry * 2;
  }
};

class KSArray : public KSObject {
 public:
  int32_t Length();

  void SetLength(int32_t length);

  int32_t Capacity();

  Object* Get(int32_t index);

  Object* Set(int32_t index, Object* value);

  Array* Elements();

  void SetElements(Array* elements);

  void IterateKSArrayBody(ObjectVisitor* visitor);

  static void Push(Handle<KSArray> self, Handle<Object> value);

  static KSArray* New(int32_t length, AllocationPolicy policy = NOT_TENURED);

  static KSArray* Cast(Object* obj);

  static constexpr int kLengthOffset = KSObject::kSize;
  static constexpr int kElementsOffset = kLengthOffset + Int32::kSize;
  static constexpr int kSize = Align(kElementsOffset + kPointerSize);

 private:
  void Push(Handle<Object> value);
};

class HeapNumber : public HeapObject {
 public:
  int64_t Value();

  static HeapNumber* New(int64_t value, AllocationPolicy policy = NOT_TENURED);

  static HeapNumber* Cast(Object* obj);

  static constexpr int kValueOffset = HeapObject::kHeaderSize;
  static constexpr int kSize = kValueOffset + sizeof(int64_t);
  static constexpr int64_t kMaxInt64 = std::numeric_limits<int64_t>::max();
  static constexpr int64_t kMinInt64 = ~kMaxInt64;

  DISABLE_DEFAULT_OP(HeapNumber)
};

class Function : public HeapObject {
 public:
  using FunctionTemplate = Handle<Object> (*)(Handle<KSArray> args,
                                              Context* context);

  String* Name();

  Array* Params();

  void* KSBody();

  FunctionTemplate Body();

  Handle<Object> Call(Handle<Object> self, Handle<KSArray> args,
                      Context* context);

  bool IsFunctionTemplate();

  void IterateFunctionBody(ObjectVisitor* visitor);

  static Function* New(String*, Array* params, void* body,
                       AllocationPolicy policy = NOT_TENURED);
  static Function* New(String* name, Array* params, FunctionTemplate body,
                       AllocationPolicy policy = NOT_TENURED);

  static Function* Cast(Object* obj);

  static constexpr int kNameOffset = HeapObject::kHeaderSize;
  static constexpr int kParamsOffset = kNameOffset + kPointerSize;
  static constexpr int kBodyOffset = kParamsOffset + kPointerSize;
  static constexpr int kSize = kBodyOffset + kPointerSize;

  static constexpr int kFunctionTemplateTag = 1;
};

class ObjectVisitor {
 public:
  virtual ~ObjectVisitor() {}

  virtual void Visit(Object** handle) = 0;

  virtual void VisitHashTableEntry(Object** key, Object** value) {
    Visit(key);
    Visit(value);
  }
};

}  // namespace internal
}  // namespace kipper
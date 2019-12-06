#include "value.hh"
#include <string>
#include "conversion.hh"
#include "heap.hh"
#include "interpreter.hh"
#include "log.hh"

using namespace kipper::internal;

#define PTR_INT(p) reinterpret_cast<uint64_t>(p)

#define ADDRESS(p) reinterpret_cast<Address>(p)

#define READ_TAG(p) (PTR_INT(p) & ~kObjectMask)

#define FIELD_ADDR(p, offset) (ADDRESS(PTR_INT(p) & kObjectMask) + (offset))

#define READ_FIELD(p, offset) \
  (*reinterpret_cast<Object**>(FIELD_ADDR(p, offset)))

#define READ_INT32_FIELD(p, offset) \
  (*reinterpret_cast<int32_t*>(FIELD_ADDR(p, offset)))

#define READ_UINT64_FIELD(p, offset) \
  (*reinterpret_cast<uint64_t*>(FIELD_ADDR(p, offset)))

#define READ_UINT64(p) (*reinterpret_cast<uint64_t*>(p))

#define WRITE_FIELD(p, offset, value) \
  *reinterpret_cast<Object**>(FIELD_ADDR(p, offset)) = value

#define WRITE_INT_PTR(p, value) *reinterpret_cast<int64_t*>(p) = value

#define WRITE_BARRIER(obj, field) Heap::WriteBarrier(obj, field)

#define CALL_WITH_GC_SUPPORT(NO_GC_FUNC)       \
  try {                                        \
    NO_GC_FUNC;                                \
  } catch (const AllocationError& alloc_err) { \
    Heap::Collect(alloc_err.space());          \
    try {                                      \
      NO_GC_FUNC;                              \
    } catch (const AllocationError&) {         \
      throw OutOfMemoryError{};                \
    }                                          \
  }

Object* Constant::true_value_ = reinterpret_cast<Object*>(Constant::kBoolTrue);
Object* Constant::false_value_ =
    reinterpret_cast<Object*>(Constant::kBoolFalse);
Object* Constant::null_value_ = reinterpret_cast<Object*>(Constant::kNull);
Object* Constant::undefined_value_ =
    reinterpret_cast<Object*>(Constant::kUndefined);

std::vector<KSObjectGetPropertyInterceptor>
    KSObject::get_property_interceptors_{};

class ToStringVisitor : public ObjectVisitor {
 public:
  explicit ToStringVisitor(std::string& builder) : builder_{builder} {}

  void Visit(Object** handle) override {
    auto obj = *handle;
    if (obj->IsString()) {
      builder_.append(String::Cast(obj)->Value());
      return;
    }
    switch (auto addr = PTR_INT(obj)) {
      case Constant::kBoolTrue:
        builder_.append("true");
        return;
      case Constant::kBoolFalse:
        builder_.append("false");
        return;
      case Constant::kNull:
        builder_.append("null");
        return;
      case Constant::kUndefined:
        builder_.append("undefined");
        return;
      default:
        if (obj->IsDouble()) {
          // TODO dtoa
          builder_.append(std::to_string(Double::Cast(obj)->Value()));
          return;
        }
        if (obj->IsInt32()) {
          builder_.append(IntToString(Int32::Cast(obj)->Value()).data());
          return;
        }
        switch (HeapObject::Cast(obj)->metadata().Type()) {
          case HeapObjectType::ARRAY:
            builder_.append("[");
            {
              auto arr_obj = Array::Cast(obj);
              for (auto i = 0, len = arr_obj->Length(); i < len; i++) {
                auto item = arr_obj->Get(i);
                Visit(&item);
                if (i < len - 1) {
                  builder_.append(", ");
                }
              }
            }
            builder_.append("]");
            return;
          case HeapObjectType::KSARRAY:
            KSArray::Cast(obj)->IterateKSArrayBody(this);
            return;
          case HeapObjectType::FUNCTION:
            builder_.append("[[function]]");
            return;
          case HeapObjectType::HEAP_NUMBER:
            builder_.append(IntToString(HeapNumber::Cast(obj)->Value()).data());
            return;
          case HeapObjectType::STRING:
            break;
          case HeapObjectType::KSOBJECT:
            builder_.append("{");
            KSObject::Cast(obj)->Elements()->IterateHashTableBody(this);
            if (KSObject::Cast(obj)->Elements()->ElementsSize()) {
              builder_.pop_back();
              builder_.pop_back();
            }
            builder_.append("}");
            return;
        }
    }
    LOG_DEBUG("UNREACHABLE obj: {}, metadata: {:b}, type: {}",
              static_cast<void*>(obj),
              HeapObject::Cast(obj)->metadata().EncodedMetadata(),
              HeapObject::Cast(obj)->metadata().Type());
    UNREACHABLE();
  }

  void VisitHashTableEntry(Object** key, Object** value) override {
    Visit(key);
    builder_.append(": ");
    Visit(value);
    builder_.append(", ");
  }

 private:
  std::string& builder_;
};

bool Object::IsNumber() { return IsDouble() || IsInt32() || IsHeapNumber(); }

bool Object::IsDouble() { return PTR_INT(this) <= Double::kDoubleLimit; }

bool Object::IsInt32() { return READ_TAG(this) == Int32::kInt32Tag; }

bool Object::IsBoolean() {
  switch (PTR_INT(this)) {
    case Constant::kBoolTrue:
    case Constant::kBoolFalse:
      return true;
    default:
      return false;
  }
  UNREACHABLE();
}

bool Object::IsTrue() { return PTR_INT(this) == Constant::kBoolTrue; }

bool Object::IsNull() { return PTR_INT(this) == Constant::kNull; }

bool Object::IsUndefined() { return PTR_INT(this) == Constant::kUndefined; }

bool Object::IsFunction() {
  return IsHeapObject() &&
         HeapObject::Cast(this)->metadata().Type() == HeapObjectType::FUNCTION;
}

bool Object::IsHeapObject() {
  return READ_TAG(this) == HeapObject::kHeapObjectTag;
}

bool Object::IsKSObject() {
  if (IsHeapObject()) {
    switch (HeapObject::Cast(this)->metadata().Type()) {
      case HeapObjectType::KSOBJECT:
      case HeapObjectType::STRING:
      case HeapObjectType::KSARRAY:
        return true;
      case HeapObjectType::HEAP_NUMBER:
      case HeapObjectType::ARRAY:
      case HeapObjectType::FUNCTION:
        return false;
    }
  }
  return false;
}

bool Object::IsString() {
  return IsHeapObject() &&
         HeapObject::Cast(this)->metadata().Type() == HeapObjectType::STRING;
}

bool Object::IsArray() {
  return IsHeapObject() &&
         HeapObject::Cast(this)->metadata().Type() == HeapObjectType::ARRAY;
}

bool kipper::internal::Object::IsKSArray() {
  return IsHeapObject() &&
         HeapObject::Cast(this)->metadata().Type() == HeapObjectType::KSARRAY;
}

bool Object::IsHeapNumber() {
  return IsHeapObject() && HeapObject::Cast(this)->metadata().Type() ==
                               HeapObjectType::HEAP_NUMBER;
}

Object* Object::ToBoolean() {
  if (IsBoolean()) {
    return this;
  }
  if (IsDouble()) {
    double value = ToDouble();
    if (value == static_cast<double>(0) || std::isnan(value)) {
      return Constant::Boolean(false);
    }
    return Constant::Boolean(true);
  }
  switch (auto addr = PTR_INT(this)) {
    case Constant::kNull:
    case Constant::kUndefined:
      return Constant::Boolean(false);
    default:
      if (IsInt32()) {
        return Constant::Boolean(Int32::Cast(this)->Value());
      }
      switch (HeapObject::Cast(this)->metadata().Type()) {
        case HeapObjectType::KSOBJECT:
          return Constant::Boolean(true);
        case HeapObjectType::STRING:
          return Constant::Boolean(String::Cast(this)->Length());
        case HeapObjectType::ARRAY:
          return Constant::Boolean(Array::Cast(this)->Length());
        case HeapObjectType::KSARRAY:
          return Constant::Boolean(KSArray::Cast(this)->Length());
        case HeapObjectType::FUNCTION:
          return Constant::Boolean(false);
        case HeapObjectType::HEAP_NUMBER:
          return Constant::Boolean(HeapNumber::Cast(this)->Value());
      }
  }
  UNREACHABLE();
  return this;
}

Object* Object::ToNumber() {
  if (IsNumber()) {
    return this;
  }
  switch (auto addr = PTR_INT(this)) {
    case Constant::kBoolTrue:
      return Int32::Make(1);
    case Constant::kBoolFalse:
      return Int32::Make(0);
    case Constant::kNull:
    case Constant::kUndefined:
      return Double::NaN();
    default:
      switch (HeapObject::Cast(this)->metadata().Type()) {
        case HeapObjectType::STRING:
          return Double::Make(StringToDouble(String::Cast(this)->Value()));
        case HeapObjectType::ARRAY:
        case HeapObjectType::KSARRAY:
        case HeapObjectType::FUNCTION:
        case HeapObjectType::KSOBJECT:
          return Double::NaN();
        case HeapObjectType::HEAP_NUMBER:
          break;
      }
  }
  UNREACHABLE();
  return this;
}

String* Object::ToString() {
  if (IsString()) {
    return String::Cast(this);
  }
  std::string builder;
  ToStringVisitor visitor{builder};
  Object* self = this;
  visitor.Visit(&self);
  return String::New(builder);
}

double Object::ToDouble() {
  if (IsDouble()) {
    return Double::Cast(this)->Value();
  }
  switch (auto addr = PTR_INT(this)) {
    case Constant::kBoolTrue:
      return 1;
    case Constant::kBoolFalse:
      return 0;
    case Constant::kNull:
    case Constant::kUndefined:
      return Double::kNaN;
    default:
      if (IsInt32()) {
        return Int32::Cast(this)->Value();
      }
      if (IsHeapNumber()) {
        return HeapNumber::Cast(this)->Value();
      }
      if (IsString()) {
        return StringToDouble(String::Cast(this)->Value());
      }
      if (IsArray() || IsFunction() || IsKSObject()) {
        return Double::kNaN;
      }
  }
  UNREACHABLE();
  return 0;
}

int32_t Object::ToInt32() {
  if (IsInt32()) {
    return Int32::Cast(this)->Value();
  }
  if (IsDouble()) {
    return DoubleToInt32(Double::Cast(this)->Value());
  }
  if (IsHeapNumber()) {
    return static_cast<int32_t>(HeapNumber::Cast(this)->Value());
  }
  switch (auto addr = PTR_INT(this)) {
    case Constant::kBoolTrue:
      return 1;
    case Constant::kBoolFalse:
      return 0;
    case Constant::kNull:
    case Constant::kUndefined:
      return 0;
    default:
      if (IsString()) {
        return StringToInt<int32_t>(String::Cast(this)->Value());
      }
      if (IsArray() || IsFunction() || IsKSObject()) {
        return 0;
      }
  }
  UNREACHABLE();
  return 0;
}

int64_t Object::ToInt64() {
  if (IsHeapNumber()) {
    return HeapNumber::Cast(this)->Value();
  }
  if (IsDouble()) {
    return DoubleToInt64(Double::Cast(this)->Value());
  }
  if (IsInt32()) {
    return Int32::Cast(this)->Value();
  }
  switch (auto addr = PTR_INT(this)) {
    case Constant::kBoolTrue:
      return 1;
    case Constant::kBoolFalse:
      return 0;
    case Constant::kNull:
    case Constant::kUndefined:
      return 0;
    default:
      if (IsString()) {
        return StringToInt<int64_t>(String::Cast(this)->Value());
      }
      if (IsArray() || IsFunction() || IsKSObject()) {
        return 0;
      }
  }
  UNREACHABLE();
  return 0;
}

std::string Object::ToStdString() {
  if (IsString()) {
    return std::string{String::Cast(this)->Value()};
  }
  std::string builder;
  ToStringVisitor visitor{builder};
  Object* self = this;
  visitor.Visit(&self);
  return builder;
}

inline bool IsSameType(Object* a, Object* b) {
  return READ_TAG(a) == READ_TAG(b);
}

bool Object::Equals(Object* that) {
  if (this == that) {  // Double, Int32, Constant
    return true;
  }
  if (IsNumber() && that->IsNumber()) {
    if (IsDouble() || that->IsDouble()) {
      return ToDouble() == that->ToDouble();
    }
    if (IsHeapNumber() || that->IsHeapNumber()) {
      return ToInt64() == that->ToInt64();
    }
  }
  if (IsString() && that->IsString()) {
    return String::Cast(this)->Value() == String::Cast(that)->Value();
  }
  return false;
}

Double* Double::Make(double value) {
  return reinterpret_cast<Double*>(*reinterpret_cast<uint64_t*>(&value));
}

Object* Double::MakeFit(double value) {
  if (Int32::Fit(value)) {
    return Int32::Make(value);
  }
  return Double::Make(value);
}

Double* Double::NaN() { return Make(kNaN); }

double Double::Value() {
  void* p = this;
  return *reinterpret_cast<double*>(&p);
}

Double* Double::Cast(Object* obj) {
  assert(obj->IsDouble());
  return reinterpret_cast<Double*>(obj);
}

int32_t Int32::Value() {
  return static_cast<int32_t>(PTR_INT(this) & kObjectMask);
}

Int32* Int32::Make(int32_t value) {
  return reinterpret_cast<Int32*>(static_cast<uint32_t>(value) |
                                  Int32::kInt32Tag);
}

Int32* Int32::Cast(Object* obj) {
  assert(obj->IsInt32());
  return reinterpret_cast<Int32*>(obj);
}

Object* Constant::Boolean(bool value) {
  return value ? true_value_ : false_value_;
}

Object* Constant::Null() { return null_value_; }

Object* Constant::Undefined() { return undefined_value_; }

Handle<Object> Constant::BooleanHandle(bool value) {
  static auto true_value_handle = &true_value_;
  static auto false_value_handle = &false_value_;
  return Handle{value ? true_value_handle : false_value_handle};
}

Handle<Object> Constant::NullHandle() {
  static auto null_handle = &null_value_;
  return Handle{null_handle};
}

Handle<Object> Constant::UndefinedHandle() {
  static auto undefined_handle = &undefined_value_;
  return Handle{undefined_handle};
}

Metadata::Metadata(HeapObject* obj)
    : metadata_{obj ? READ_UINT64_FIELD(obj, HeapObject::kMetadataOffset) : 0} {
}

Address HeapObject::address() { return ADDRESS(PTR_INT(this) & kObjectMask); }

Metadata HeapObject::metadata() { return Metadata{this}; }

void HeapObject::set_metadata(Metadata metadata) {
  READ_UINT64_FIELD(this, kMetadataOffset) = metadata.EncodedMetadata();
}

int HeapObject::Size() {
  switch (metadata().Type()) {
    case KSOBJECT:
      return KSObject::kSize;
    case STRING:
      return String::EnsureSize(String::Cast(this)->Length());
    case ARRAY:
      return Array::EnsureSize(Array::Cast(this)->Length());
    case KSARRAY:
      return KSArray::kSize;
    case HEAP_NUMBER:
      return HeapNumber::kSize;
    case FUNCTION:
      return Function::kSize;
  }
  UNREACHABLE();
  return 0;
}

void HeapObject::IterateBody(ObjectVisitor* visitor) {
  switch (metadata().Type()) {
    case ARRAY:
      Array::Cast(this)->IterateArrayBody(visitor);
      return;
    case KSARRAY:
      KSArray::Cast(this)->IterateKSArrayBody(visitor);

      // fall through

    case KSOBJECT:
    case STRING:
      KSObject::Cast(this)->IterateKSObjectBody(visitor);
      return;
    case FUNCTION:
      Function::Cast(this)->IterateFunctionBody(visitor);
      return;
    case HEAP_NUMBER:
      return;
  }
}

HeapObject* HeapObject::Make(Address addr) {
  return reinterpret_cast<HeapObject*>(PTR_INT(addr) | kHeapObjectTag);
}

HeapObject* HeapObject::Cast(Object* obj) {
  assert(obj->IsHeapObject());
  return static_cast<HeapObject*>(obj);
}

Object* KSObject::GetProperty(Object* key) {
  auto str_key = key->ToString();
  for (KSObjectGetPropertyInterceptor interceptor :
       get_property_interceptors_) {
    if (auto result = interceptor(this, str_key)) {
      return result.Get();
    }
  }
  if (auto value = Elements()->Search(str_key)) {
    return value;
  }
  return Constant::Undefined();
}

HashTable* KSObject::Elements() {
  return HashTable::Cast(READ_FIELD(this, kElementsOffset));
}

void KSObject::SetElements(HashTable* elements) {
  WRITE_FIELD(this, kElementsOffset, elements);
}

void KSObject::IterateKSObjectBody(ObjectVisitor* visitor) {
  visitor->Visit(&READ_FIELD(this, kElementsOffset));
}

void KSObject::SetProperty(Handle<KSObject> self, Handle<Object> key,
                           Handle<Object> value) {
  CALL_WITH_GC_SUPPORT(self->SetProperty(key.Get(), value.Get()));
}

KSObject* KSObject::New(int32_t elements_size, AllocationPolicy policy) {
  return Cast(Heap::AllocateKSObject(elements_size, policy));
}

KSObject* KSObject::Cast(Object* obj) {
  assert(obj->IsKSObject());
  return static_cast<KSObject*>(obj);
}

void KSObject::AddGetPropertyInterceptor(
    KSObjectGetPropertyInterceptor interceptor) {
  get_property_interceptors_.push_back(interceptor);
}

Object* KSObject::SetProperty(Object* key, Object* value) {
  auto elements = Elements();
  auto table = elements->Insert(key->ToString(), value);
  if (table == elements) {
    return value;
  }
  SetElements(table);
  return value;
}

String* String::New(std::string_view value, AllocationPolicy policy) {
  auto result = Cast(Heap::AllocateString(value.size(), policy));
  result->Content(value);
  return result;
}

String* String::NewSymbol(std::string_view value) {
  return String::Cast(Heap::LookupSymbol(value));
}

String* String::Cast(Object* obj) {
  assert(obj->IsString());
  return reinterpret_cast<String*>(obj);
}

int32_t String::Length() {
  assert(IsString());
  return READ_INT32_FIELD(this, kLengthOffset);
}

void String::Content(std::string_view value) {
  assert(value.size() == Length());
  std::memcpy(FIELD_ADDR(this, kBytesOffset), value.data(), value.size());
}

int String::Hash() { return Hash(Value()); }

int String::Hash(std::string_view value) {
  auto length = value.size();
  if (length == 0) {
    return 0;
  }
  if (length == 1) {
    return static_cast<int>(value[0]);
  }
  int hash = 0;
  for (auto ch : value) {
    hash += static_cast<int>(ch) * 31;
  }
  return hash;
}

String* String::Concat(String* that) {
  if (!that->Length()) {
    return this;
  }
  auto result = Heap::AllocateString(Length() + that->Length());
  std::memcpy(FIELD_ADDR(result, kBytesOffset), FIELD_ADDR(this, kBytesOffset),
              Length());
  std::memcpy(
      reinterpret_cast<char*>(FIELD_ADDR(result, kBytesOffset)) + Length(),
      FIELD_ADDR(that, kBytesOffset), that->Length());
  return Cast(result);
}

std::string_view String::Value() {
  return std::string_view(
      reinterpret_cast<const char*>(FIELD_ADDR(this, kBytesOffset)), Length());
}

void String::Length(int32_t length) {
  READ_INT32_FIELD(this, kLengthOffset) = length;
}

int32_t Array::Length() { return READ_INT32_FIELD(this, kLengthOffset); }

void Array::SetLength(int32_t length) {
  READ_INT32_FIELD(this, kLengthOffset) = length;
}

Object* Array::Get(int32_t index) {
  assert(index < Length());
  return READ_FIELD(this, kElementsOffset + kPointerSize * index);
}

Object* Array::Set(int32_t index, Object* value) {
  if (index < Length()) {
    WRITE_BARRIER(this, value);
    READ_FIELD(this, kElementsOffset + kPointerSize * index) = value;
    return value;
  }
  return Constant::Undefined();
}

void Array::Copy(Array* that) {
  assert(Length() >= that->Length());
  std::memcpy(FIELD_ADDR(this, kElementsOffset),
              FIELD_ADDR(that, kElementsOffset), that->Length() * kPointerSize);
}

void Array::IterateArrayBody(ObjectVisitor* visitor) {
  for (auto i = 0, len = Length(); i < len; i++) {
    visitor->Visit(&READ_FIELD(this, kElementsOffset + i * kPointerSize));
  }
}

Array* Array::New(int32_t length, AllocationPolicy policy) {
  return Cast(Heap::AllocateArray(length, policy));
}

Array* Array::Cast(Object* obj) {
  assert(obj->IsArray());
  return reinterpret_cast<Array*>(obj);
}

Object** Array::GetHandle(int32_t index) {
  assert(index < Length());
  return &READ_FIELD(this, kElementsOffset + kPointerSize * index);
}

HashTable* HashTable::Insert(String* key, Object* value) {
  int hash = key->Hash();
  auto entry = FindEntry(key, hash);
  if (entry != -1) {
    Set(EntryToIndex(entry) + 1, value);
    return this;
  }
  auto table = AddElement(1);
  auto insertion_index = table->FindInsertionIndex(hash);
  table->SetEntry(insertion_index, key, value);
  return table;
}

Object* HashTable::Search(String* key) {
  auto entry = FindEntry(key, key->Hash());
  if (entry != -1) {
    return Get(EntryToIndex(entry) + 1);
  }
  return nullptr;
}

bool HashTable::Delete(String* key) {
  auto entry = FindEntry(key, key->Hash());
  if (entry != -1) {
    SetEntry(EntryToIndex(entry), Constant::Undefined(), Constant::Undefined());
    SetElementsSize(ElementsSize() - 1);
    return true;
  }
  return false;
}

void HashTable::IterateHashTableBody(ObjectVisitor* visitor) {
  auto elements_size = ElementsSize();
  if (elements_size == 0) {
    return;
  }
  auto capacity = Capacity();
  int passed_elements_size = 0;
  for (auto i = 0; i < capacity; i++) {
    auto index = EntryToIndex(i);
    if (auto key = GetHandle(index); !(*key)->IsUndefined()) {
      visitor->VisitHashTableEntry(key, GetHandle(index + 1));
      if (++passed_elements_size == elements_size) {
        return;
      }
    }
  }
}

HashTable* HashTable::Cast(Object* obj) {
  assert(obj->IsArray());
  return static_cast<HashTable*>(obj);
}

HashTable* HashTable::AddElement(int count) {
  int capacity = Capacity();
  int new_elements_size = ElementsSize() + count;
  if (new_elements_size + (new_elements_size >> 2) <= capacity) {
    SetElementsSize(new_elements_size);
    return this;
  }
  auto new_table = Cast(Heap::AllocateHashTableNoGC(new_elements_size * 2));
  new_table->SetElementsSize(new_elements_size);
  for (int i = 0; i < capacity; i++) {
    auto from_index = EntryToIndex(i);
    auto key = String::Cast(Get(from_index));
    if (!key->IsUndefined()) {
      auto insertion_index = new_table->FindInsertionIndex(key->Hash());
      new_table->SetEntry(insertion_index, key, Get(from_index + 1));
    }
  }
  return new_table;
}

int HashTable::FindInsertionIndex(int hash) {
  auto capacity = Capacity();
  auto index = EntryToIndex(Location(hash, 0, capacity));
  if (EntryKeyAt(index)->IsUndefined()) {
    return index;
  }
  for (int i = 1;; i++) {
    index = EntryToIndex(Location(hash, i, capacity));
    if (EntryKeyAt(index)->IsUndefined()) {
      return index;
    }
  }
  return index;
}

int HashTable::FindEntry(String* key, int hash) {
  auto elements_size = ElementsSize();
  if (elements_size == 0) {
    return -1;
  }
  auto capacity = Capacity();
  auto entry = Location(hash, 0, capacity);
  int passed_elements_size = 0;
  if (auto entry_key = EntryKeyAt(EntryToIndex(entry));
      !entry_key->IsUndefined()) {
    if (entry_key->Equals(key)) {
      return entry;
    }
    passed_elements_size++;
  }
  for (auto i = 1; i < capacity; i++) {
    entry = Location(hash, i, capacity);
    if (auto entry_key = EntryKeyAt(EntryToIndex(entry));
        !entry_key->IsUndefined()) {
      if (entry_key->Equals(key)) {
        return entry;
      }
      if (++passed_elements_size == elements_size) {
        return -1;
      }
    }
  }
  return -1;
}

void HashTable::SetEntry(int index, Object* key, Object* value) {
  assert(index + 1 < Length());
  Set(index, key);
  Set(index + 1, value);
}

int32_t KSArray::Length() {
  assert(IsKSArray());
  return READ_INT32_FIELD(this, kLengthOffset);
}

void KSArray::SetLength(int32_t length) {
  assert(IsKSArray());
  READ_INT32_FIELD(this, kLengthOffset) = length;
}

int32_t KSArray::Capacity() {
  assert(IsKSArray());
  return Elements()->Length();
}

Object* KSArray::Get(int32_t index) {
  assert(IsKSArray());
  if (index < Length()) {
    return Elements()->Get(index);
  }
  return Constant::Undefined();
}

Object* KSArray::Set(int32_t index, Object* value) {
  assert(IsKSArray());
  return Elements()->Set(index, value);
}

Array* KSArray::Elements() {
  return Array::Cast(READ_FIELD(this, kElementsOffset));
}

void KSArray::SetElements(Array* elements) {
  WRITE_FIELD(this, kElementsOffset, elements);
  WRITE_BARRIER(this, elements);
}

void KSArray::IterateKSArrayBody(ObjectVisitor* visitor) {
  visitor->Visit(&READ_FIELD(this, kElementsOffset));
}

void KSArray::Push(Handle<KSArray> self, Handle<Object> value) {
  CALL_WITH_GC_SUPPORT(self->Push(value));
}

KSArray* KSArray::New(int32_t length, AllocationPolicy policy) {
  return Cast(Heap::AllocateKSArray(length, policy));
}

KSArray* KSArray::Cast(Object* obj) {
  assert(obj->IsKSArray());
  return static_cast<KSArray*>(obj);
}

void KSArray::Push(Handle<Object> value) {
  auto current_length = Length();
  if (current_length >= Capacity()) {
    int length = current_length + 1 + (current_length >> 1);
    auto new_array = Array::Cast(Heap::AllocateArrayNoGC(length));
    new_array->Copy(Elements());
    SetElements(new_array);
  }
  SetLength(current_length + 1);
  Elements()->Set(current_length, *value);
}

HeapNumber* HeapNumber::New(int64_t value, AllocationPolicy policy) {
  auto result = Heap::AllocateHeapNumber(policy);
  *reinterpret_cast<int64_t*>(FIELD_ADDR(result, kValueOffset)) = value;
  return Cast(result);
}

int64_t HeapNumber::Value() {
  return *reinterpret_cast<int64_t*>(FIELD_ADDR(this, kValueOffset));
}

HeapNumber* HeapNumber::Cast(Object* obj) {
  assert(obj->IsHeapNumber());
  return static_cast<HeapNumber*>(obj);
}

String* Function::Name() { return String::Cast(READ_FIELD(this, kNameOffset)); }

Array* Function::Params() {
  return Array::Cast(READ_FIELD(this, kParamsOffset));
}

void* Function::KSBody() {
  assert(!IsFunctionTemplate());
  return READ_FIELD(this, kBodyOffset);
}

Function::FunctionTemplate Function::Body() {
  assert(IsFunctionTemplate());
  return reinterpret_cast<FunctionTemplate>(
      PTR_INT(READ_FIELD(this, kBodyOffset)) - kFunctionTemplateTag);
}

bool Function::IsFunctionTemplate() {
  return READ_UINT64_FIELD(this, kBodyOffset) & kFunctionTemplateTag;
}

Handle<Object> Function::Call(Handle<Object> self, Handle<KSArray> args,
                              Context* context) {
  return Kipper::interpreter()->Call(self, Handle<Object>{this}, args, context);
}

void Function::IterateFunctionBody(ObjectVisitor* visitor) {
  visitor->Visit(&READ_FIELD(this, kNameOffset));
  visitor->Visit(&READ_FIELD(this, kParamsOffset));
  visitor->Visit(&READ_FIELD(this, kBodyOffset));
}

Function* Function::New(String* name, Array* params, void* body,
                        AllocationPolicy policy) {
  auto fn = Heap::AllocateFunction(policy);
  WRITE_FIELD(fn, kNameOffset, name);
  WRITE_FIELD(fn, kParamsOffset, params);
  WRITE_FIELD(fn, kBodyOffset, reinterpret_cast<Object*>(body));

  WRITE_BARRIER(fn, name);
  WRITE_BARRIER(fn, params);

  return Cast(fn);
}

Function* Function::New(String* name, Array* params, FunctionTemplate body,
                        AllocationPolicy policy) {
  auto fn = Heap::AllocateFunction(policy);
  WRITE_FIELD(fn, kNameOffset, name);
  WRITE_FIELD(fn, kParamsOffset, params);
  WRITE_FIELD(fn, kBodyOffset,
              reinterpret_cast<Object*>(PTR_INT(body) | kFunctionTemplateTag));

  WRITE_BARRIER(fn, name);
  WRITE_BARRIER(fn, params);
  return Cast(fn);
}

Function* Function::Cast(Object* obj) {
  assert(obj->IsFunction());
  return static_cast<Function*>(obj);
}

uint8_t Metadata::Age() const {
  return static_cast<uint8_t>((metadata_ & AgeMask) >> AgeBitsOffset);
}

bool Metadata::IsForwarding() const { return metadata_ & ForwardingMask; }

bool Metadata::IsRemembered() const { return metadata_ & RememberedTagMask; }

bool Metadata::IsMarked() const { return metadata_ & MarkedTagMask; }

HeapObject* Metadata::Forwarding() const {
  assert(IsForwarding());
  return HeapObject::Make(
      reinterpret_cast<Address>(metadata_ & ForwardingMask));
}

HeapObjectType Metadata::Type() const {
  auto encoded_metadata = metadata_ & Metadata::TypeMask;
  return static_cast<HeapObjectType>(encoded_metadata >>
                                     Metadata::TypeBitsOffset);
}

void Metadata::IncrementAge() {
  auto age_bits = (static_cast<uint64_t>(Age() + 1) << AgeBitsOffset);
  metadata_ = age_bits | (metadata_ & ~AgeMask);
  assert(static_cast<uint8_t>(metadata_ >> 48) >= HeapObjectType::KSOBJECT &&
         static_cast<uint8_t>(metadata_ >> 48) <= HeapObjectType::FUNCTION);
}

void Metadata::Forwarding(Address addr) {
  assert(!IsForwarding());
  metadata_ |= (PTR_INT(addr) & ForwardingMask);
}

void Metadata::Remember() { metadata_ |= RememberedTagMask; }

void Metadata::Mark() { metadata_ |= MarkedTagMask; }

void Metadata::ResetForwarding() { metadata_ &= ~ForwardingMask; }

void Metadata::ResetRemembered() { metadata_ &= ~RememberedTagMask; }

void Metadata::ResetMarked() { metadata_ &= ~MarkedTagMask; }

void Metadata::set_type(HeapObjectType type) {
  uint64_t encoded_type = static_cast<uint64_t>(type)
                          << Metadata::TypeBitsOffset;
  metadata_ = (metadata_ & ~TypeMask) | encoded_type;
}

uint64_t Metadata::EncodedMetadata() const { return metadata_; }

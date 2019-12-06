#include <cmath>
#include <iostream>
#include <numeric>
#include "unittest.hh"

using namespace kipper;

class ValueTest : public ::testing::Test {
 protected:
  ValueTest() { kipper::Kipper::Initialize(); }
};

TEST_F(ValueTest, ConstructNumber) {
  {
    auto zero = Number::New(0);
    EXPECT_TRUE(zero->IsNumber());
    EXPECT_FALSE(zero->IsBoolean());
    EXPECT_FALSE(zero->IsString());
    EXPECT_FALSE(zero->IsArray());
    EXPECT_FALSE(zero->IsNull());
    EXPECT_FALSE(zero->IsUndefined());
    EXPECT_FALSE(zero->IsFunction());
    EXPECT_FALSE(zero->IsObject());

    EXPECT_EQ(zero->Double(), 0);
    EXPECT_EQ(zero->Int32(), 0);
    EXPECT_EQ(zero->Int64(), 0);
  }

  {
    auto nan = Number::New(kNaN);
    EXPECT_TRUE(std::isnan(nan->Double()));
    EXPECT_EQ(nan->Int32(), 0);
    EXPECT_EQ(nan->Int64(), 0);
  }

  {
    auto pie = Number::New(3.14);
    EXPECT_EQ(pie->Double(), 3.14);
    EXPECT_EQ(pie->Int32(), 3);
    EXPECT_EQ(pie->Int64(), 3);
  }

  {
    auto n_pie = Number::New(-3.14);
    EXPECT_EQ(n_pie->Double(), -3.14);
    EXPECT_EQ(n_pie->Int32(), -3);
    EXPECT_EQ(n_pie->Int64(), -3);
  }

  {
    auto double_min = Number::New(kDoubleMin);
    EXPECT_EQ(double_min->Double(), kDoubleMin);
    EXPECT_EQ(double_min->Int32(), 0);
    EXPECT_EQ(double_min->Int64(), 0);
  }

  {
    auto double_max = Number::New(kDoubleMax);
    EXPECT_EQ(double_max->Double(), kDoubleMax);
    EXPECT_EQ(double_max->Int32(), 0x7fffffff);
    EXPECT_EQ(double_max->Int64(), 0x7fffffffffffffff);
  }

  {
    auto int32_min = Number::New(kInt32Min);
    EXPECT_EQ(int32_min->Double(), kInt32Min);
    EXPECT_EQ(int32_min->Int32(), kInt32Min);
    EXPECT_EQ(int32_min->Int64(), kInt32Min);
  }

  {
    auto int32_max = Number::New(kInt32Max);
    EXPECT_EQ(int32_max->Double(), kInt32Max);
    EXPECT_EQ(int32_max->Int32(), kInt32Max);
    EXPECT_EQ(int32_max->Int64(), kInt32Max);
  }

  {
    auto int64_min = Number::New(kInt64Min);
    EXPECT_EQ(int64_min->Int64(), kInt64Min);
    EXPECT_EQ(int64_min->Double(), static_cast<double>(kInt64Min));
    EXPECT_EQ(int64_min->Int32(), static_cast<int32_t>(kInt64Min));
  }

  {
    auto int64_max = Number::New(kInt64Max);
    EXPECT_EQ(int64_max->Double(), static_cast<double>(kInt64Max));
    EXPECT_EQ(int64_max->Int32(), static_cast<int32_t>(kInt64Max));
    EXPECT_EQ(int64_max->Int64(), kInt64Max);
  }
}

TEST_F(ValueTest, ConstructBoolean) {
  auto true_value = Boolean::New(true);
  EXPECT_FALSE(true_value->IsNumber());
  EXPECT_TRUE(true_value->IsBoolean());
  EXPECT_FALSE(true_value->IsString());
  EXPECT_FALSE(true_value->IsArray());
  EXPECT_FALSE(true_value->IsNull());
  EXPECT_FALSE(true_value->IsUndefined());
  EXPECT_FALSE(true_value->IsFunction());
  EXPECT_FALSE(true_value->IsObject());

  EXPECT_EQ(true_value->Value(), true);

  EXPECT_EQ(Boolean::New(false)->Value(), false);
}

TEST_F(ValueTest, ConstructString) {
  auto empty_string = String::New("");

  EXPECT_FALSE(empty_string->IsNumber());
  EXPECT_FALSE(empty_string->IsBoolean());
  EXPECT_TRUE(empty_string->IsString());
  EXPECT_FALSE(empty_string->IsArray());
  EXPECT_FALSE(empty_string->IsNull());
  EXPECT_FALSE(empty_string->IsUndefined());
  EXPECT_FALSE(empty_string->IsFunction());
  EXPECT_TRUE(empty_string->IsObject());

  EXPECT_EQ(empty_string->StringView(), "");
  EXPECT_EQ(empty_string->Length(), 0);

  constexpr std::string_view hello_world{"hello world"};
  auto hello_world_string = String::New(hello_world);
  EXPECT_EQ(hello_world_string->StringView(), hello_world);
  EXPECT_EQ(hello_world_string->Length(), 11);

  auto concat_empty_string = hello_world_string->Concat(empty_string);
  EXPECT_EQ(concat_empty_string->StringView(), hello_world);
  EXPECT_EQ(concat_empty_string->Length(), 11);

  constexpr std::string_view string_with_eof{"hello\0world", 11};
  auto string_with_eof_string = String::New(string_with_eof);
  EXPECT_EQ(string_with_eof_string->StringView(), string_with_eof);
  EXPECT_EQ(string_with_eof_string->Length(), 11);

  auto hello_world_concat_with_eof =
      hello_world_string->Concat(string_with_eof_string);

  EXPECT_EQ(hello_world_concat_with_eof->StringView(),
            std::string_view("hello worldhello\0world", 22));
  EXPECT_EQ(hello_world_concat_with_eof->Length(), 22);
}

TEST_F(ValueTest, ConstructArray) {
  auto empty_array = Array::New(0);

  EXPECT_FALSE(empty_array->IsNumber());
  EXPECT_FALSE(empty_array->IsBoolean());
  EXPECT_FALSE(empty_array->IsString());
  EXPECT_TRUE(empty_array->IsArray());
  EXPECT_FALSE(empty_array->IsNull());
  EXPECT_FALSE(empty_array->IsUndefined());
  EXPECT_FALSE(empty_array->IsFunction());
  EXPECT_TRUE(empty_array->IsObject());

  EXPECT_EQ(empty_array->Length(), 0);

  auto one_element_array = Array::New(1);

  EXPECT_EQ(one_element_array->Length(), 1);
}

TEST_F(ValueTest, ConstructFunction) {
  std::string_view params[2]{"param1", "param2"};
  auto fn = Function::New(
      "fn1", 2, params,
      [](Handle<Array> params, Context* context) -> Handle<Value> {
        return Undefined();
      });

  EXPECT_FALSE(fn->IsNumber());
  EXPECT_FALSE(fn->IsBoolean());
  EXPECT_FALSE(fn->IsString());
  EXPECT_FALSE(fn->IsArray());
  EXPECT_FALSE(fn->IsNull());
  EXPECT_FALSE(fn->IsUndefined());
  EXPECT_TRUE(fn->IsFunction());
  EXPECT_FALSE(fn->IsObject());
}

TEST_F(ValueTest, ConstructUndefined) {
  auto undefined = kipper::Undefined();
  EXPECT_FALSE(undefined->IsNumber());
  EXPECT_FALSE(undefined->IsBoolean());
  EXPECT_FALSE(undefined->IsString());
  EXPECT_FALSE(undefined->IsArray());
  EXPECT_FALSE(undefined->IsNull());
  EXPECT_TRUE(undefined->IsUndefined());
  EXPECT_FALSE(undefined->IsFunction());
  EXPECT_FALSE(undefined->IsObject());
}

TEST_F(ValueTest, ConstructNull) { EXPECT_TRUE(kipper::Null()->IsNull()); }

TEST_F(ValueTest, ConstructObject) {
  auto empty_object = Object::New(0);
  EXPECT_FALSE(empty_object->IsNumber());
  EXPECT_FALSE(empty_object->IsBoolean());
  EXPECT_FALSE(empty_object->IsString());
  EXPECT_FALSE(empty_object->IsArray());
  EXPECT_FALSE(empty_object->IsNull());
  EXPECT_FALSE(empty_object->IsUndefined());
  EXPECT_FALSE(empty_object->IsFunction());
  EXPECT_TRUE(empty_object->IsObject());
  EXPECT_EQ(empty_object->GetProperty(String::New("123")), Undefined());
  empty_object->SetProperty(String::New("123"), Number::New(2.2));
  EXPECT_EQ(empty_object->GetProperty(String::New("123")), Number::New(2.2));
}

TEST_F(ValueTest, ToNumber) {
  EXPECT_EQ(Number::New(0)->ToNumber()->Double(), 0);
  EXPECT_EQ(Number::New(0)->ToNumber()->Int32(), 0);
  EXPECT_EQ(Number::New(0)->ToNumber()->Int64(), 0);

  EXPECT_EQ(Number::New(0.1)->ToNumber()->Double(), 0.1);
  EXPECT_EQ(Number::New(0.1)->ToNumber()->Int32(), 0);
  EXPECT_EQ(Number::New(0.1)->ToNumber()->Int64(), 0);

  EXPECT_EQ(Number::New(-0.1)->ToNumber()->Double(), -0.1);
  EXPECT_EQ(Number::New(-0.1)->ToNumber()->Int32(), 0);
  EXPECT_EQ(Number::New(-0.1)->ToNumber()->Int64(), 0);

  EXPECT_TRUE(std::isnan(Number::New(kNaN)->ToNumber()->Double()));
  EXPECT_EQ(Number::New(kNaN)->ToNumber()->Int32(), 0);
  EXPECT_EQ(Number::New(kNaN)->ToNumber()->Int64(), 0);

  EXPECT_EQ(Number::New(kDoubleMin)->ToNumber()->Double(), kDoubleMin);
  EXPECT_EQ(Number::New(kDoubleMin)->ToNumber()->Int32(),
            static_cast<int32_t>(kDoubleMin));
  EXPECT_EQ(Number::New(kDoubleMin)->ToNumber()->Int64(),
            static_cast<int64_t>(kDoubleMin));

  EXPECT_EQ(Number::New(kDoubleMax)->ToNumber()->Double(), kDoubleMax);
  EXPECT_EQ(Number::New(kDoubleMax)->ToNumber()->Int32(), kInt32Max);
  EXPECT_EQ(Number::New(kDoubleMax)->ToNumber()->Int64(), kInt64Max);

  EXPECT_EQ(Number::New(kInt32Min)->ToNumber()->Double(), kInt32Min);
  EXPECT_EQ(Number::New(kInt32Min)->ToNumber()->Int32(), kInt32Min);
  EXPECT_EQ(Number::New(kInt32Min)->ToNumber()->Int64(), kInt32Min);

  EXPECT_EQ(Number::New(kInt32Max)->ToNumber()->Double(), kInt32Max);
  EXPECT_EQ(Number::New(kInt32Max)->ToNumber()->Int32(), kInt32Max);
  EXPECT_EQ(Number::New(kInt32Max)->ToNumber()->Int64(), kInt32Max);

  EXPECT_EQ(Number::New(kInt64Min)->ToNumber()->Double(),
            static_cast<double>(kInt64Min));
  EXPECT_EQ(Number::New(kInt64Min)->ToNumber()->Int32(),
            static_cast<int32_t>(kInt64Min));
  EXPECT_EQ(Number::New(kInt64Min)->ToNumber()->Int64(), kInt64Min);

  EXPECT_EQ(Number::New(kInt64Max)->ToNumber()->Double(),
            static_cast<double>(kInt64Max));
  EXPECT_EQ(Number::New(kInt64Max)->ToNumber()->Int32(),
            static_cast<int32_t>(kInt64Max));
  EXPECT_EQ(Number::New(kInt64Max)->ToNumber()->Int64(), kInt64Max);

  EXPECT_EQ(Boolean::New(true)->ToNumber()->Double(), 1);
  EXPECT_EQ(Boolean::New(true)->ToNumber()->Int32(), 1);
  EXPECT_EQ(Boolean::New(true)->ToNumber()->Int64(), 1);

  EXPECT_EQ(Boolean::New(false)->ToNumber()->Double(), 0);
  EXPECT_EQ(Boolean::New(false)->ToNumber()->Int32(), 0);
  EXPECT_EQ(Boolean::New(false)->ToNumber()->Int64(), 0);

  {
    auto str_num = String::New("")->ToNumber();
    EXPECT_TRUE(std::isnan(str_num->Double()));
    EXPECT_EQ(str_num->Int32(), 0);
    EXPECT_EQ(str_num->Int64(), 0);
  }

  {
    auto str_num = String::New("hello world")->ToNumber();
    EXPECT_TRUE(std::isnan(str_num->Double()));
    EXPECT_EQ(str_num->Int32(), 0);
    EXPECT_EQ(str_num->Int64(), 0);
  }

  {
    auto str_num = String::New("0")->ToNumber();
    EXPECT_EQ(str_num->Double(), 0);
    EXPECT_EQ(str_num->Int32(), 0);
    EXPECT_EQ(str_num->Int64(), 0);
  }

  {
    auto str_num = String::New("1")->ToNumber();
    EXPECT_EQ(str_num->Double(), 1);
    EXPECT_EQ(str_num->Int32(), 1);
    EXPECT_EQ(str_num->Int64(), 1);
  }

  {
    auto str_num = String::New("-1")->ToNumber();
    EXPECT_EQ(str_num->Double(), -1);
    EXPECT_EQ(str_num->Int32(), -1);
    EXPECT_EQ(str_num->Int64(), -1);
  }

  {
    auto str_num = String::New("2.2250738585072014e-308")->ToNumber();
    EXPECT_DOUBLE_EQ(str_num->Double(), kDoubleMin);
    EXPECT_EQ(str_num->Int32(), static_cast<int32_t>(kDoubleMin));
    EXPECT_EQ(str_num->Int64(), static_cast<int64_t>(kDoubleMin));
  }

  {
    auto str_num = String::New(std::to_string(kDoubleMax))->ToNumber();
    EXPECT_EQ(str_num->Double(), kDoubleMax);
    EXPECT_EQ(str_num->Int32(), kInt32Max);
    EXPECT_EQ(str_num->Int64(), kInt64Max);
  }

  {
    auto str_num = String::New(std::to_string(kInt32Min))->ToNumber();
    EXPECT_EQ(str_num->Double(), kInt32Min);
    EXPECT_EQ(str_num->Int32(), kInt32Min);
    EXPECT_EQ(str_num->Int64(), kInt32Min);
  }

  {
    auto str_num = String::New(std::to_string(kInt32Max))->ToNumber();
    EXPECT_EQ(str_num->Double(), kInt32Max);
    EXPECT_EQ(str_num->Int32(), kInt32Max);
    EXPECT_EQ(str_num->Int64(), kInt32Max);
  }

  {
    auto str_num = String::New(std::to_string(kInt64Min))->ToNumber();
    EXPECT_EQ(str_num->Double(), static_cast<double>(kInt64Min));
    EXPECT_EQ(str_num->Int32(), kInt32Min);
    EXPECT_EQ(str_num->Int64(), kInt64Min);
  }

  {
    auto str_num = String::New(std::to_string(kInt64Max))->ToNumber();
    EXPECT_EQ(str_num->Double(), static_cast<double>(kInt64Max));
    EXPECT_EQ(str_num->Int32(), kInt32Max);
    EXPECT_EQ(str_num->Int64(), kInt64Max);
  }

  {
    auto array_num = Array::New(1)->ToNumber();
    EXPECT_TRUE(std::isnan(array_num->Double()));
    EXPECT_EQ(array_num->Int32(), 0);
    EXPECT_EQ(array_num->Int64(), 0);
  }

  {
    std::string_view params[2]{"param1", "param2"};
    auto fn = Function::New(
        "fn1", 2, params,
        [](Handle<Array> params, Context* context) -> Handle<Value> {
          return Undefined();
        });
    EXPECT_TRUE(std::isnan(fn->ToNumber()->Double()));
  }

  EXPECT_TRUE(std::isnan(Undefined()->ToNumber()->Double()));
  EXPECT_TRUE(std::isnan(Null()->ToNumber()->Double()));
}

TEST_F(ValueTest, ToBoolean) {
  EXPECT_EQ(Number::New(0)->ToBoolean()->Value(), false);
  EXPECT_EQ(Number::New(0.0)->ToBoolean()->Value(), false);
  EXPECT_EQ(Number::New(0.1)->ToBoolean()->Value(), true);
  EXPECT_EQ(Number::New(-0.1)->ToBoolean()->Value(), true);
  EXPECT_EQ(Number::New(1)->ToBoolean()->Value(), true);
  EXPECT_EQ(Number::New(2)->ToBoolean()->Value(), true);
  EXPECT_EQ(Number::New(-1)->ToBoolean()->Value(), true);
  EXPECT_EQ(Number::New(-2)->ToBoolean()->Value(), true);
  EXPECT_EQ(Number::New(-2)->ToBoolean()->Value(), true);
  EXPECT_EQ(Number::New(kNaN)->ToBoolean()->Value(), false);
  EXPECT_EQ(Number::New(kDoubleMin)->ToBoolean()->Value(), true);
  EXPECT_EQ(Number::New(kDoubleMax)->ToBoolean()->Value(), true);
  EXPECT_EQ(Number::New(kInt32Min)->ToBoolean()->Value(), true);
  EXPECT_EQ(Number::New(kInt32Max)->ToBoolean()->Value(), true);
  EXPECT_EQ(Number::New(kInt64Min)->ToBoolean()->Value(), true);
  EXPECT_EQ(Number::New(kInt64Max)->ToBoolean()->Value(), true);

  EXPECT_EQ(Boolean::New(true)->ToBoolean()->Value(), true);
  EXPECT_EQ(Boolean::New(false)->ToBoolean()->Value(), false);

  EXPECT_EQ(String::New("")->ToBoolean()->Value(), false);
  EXPECT_EQ(String::New("hello world")->ToBoolean()->Value(), true);

  EXPECT_EQ(Array::New(0)->ToBoolean()->Value(), false);
  EXPECT_EQ(Array::New(1)->ToBoolean()->Value(), true);

  {
    std::string_view params[2]{"param1", "param2"};
    auto fn = Function::New(
        "fn1", 2, params,
        [](Handle<Array> params, Context* context) -> Handle<Value> {
          return Undefined();
        });
    EXPECT_EQ(fn->ToBoolean()->Value(), false);
  }

  EXPECT_EQ(Undefined()->ToBoolean()->Value(), false);
  EXPECT_EQ(Null()->ToBoolean()->Value(), false);
}

TEST_F(ValueTest, ToString) {
  EXPECT_EQ(Number::New(0)->ToString()->StringView(), "0");
  EXPECT_EQ(Number::New(kNaN)->ToString()->StringView(), std::to_string(kNaN));
  EXPECT_EQ(Number::New(kDoubleMin)->ToString()->StringView(),
            std::to_string(kDoubleMin));
  EXPECT_EQ(Number::New(kDoubleMax)->ToString()->StringView(),
            std::to_string(kDoubleMax));
  EXPECT_EQ(Number::New(kInt32Min)->ToString()->StringView(),
            std::to_string(kInt32Min));
  EXPECT_EQ(Number::New(kInt32Max)->ToString()->StringView(),
            std::to_string(kInt32Max));
  EXPECT_EQ(Number::New(kInt64Min)->ToString()->StringView(),
            std::to_string(kInt64Min));
  EXPECT_EQ(Number::New(kInt64Max)->ToString()->StringView(),
            std::to_string(kInt64Max));

  EXPECT_EQ(Boolean::New(true)->ToString()->StringView(),
            std::string_view{"true"});
  EXPECT_EQ(Boolean::New(false)->ToString()->StringView(),
            std::string_view{"false"});

  EXPECT_EQ(String::New("")->ToString()->StringView(), "");
  EXPECT_EQ(String::New("hello world")->ToString()->StringView(),
            "hello world");

  EXPECT_EQ(Array::New(0)->ToString()->StringView(), std::string_view{"[]"});
  EXPECT_EQ(Array::New(1)->ToString()->StringView(),
            std::string_view{"[undefined]"});

  {
    std::string_view params[2]{"param1", "param2"};
    auto fn = Function::New(
        "fn1", 2, params,
        [](Handle<Array> params, Context* context) -> Handle<Value> {
          return Undefined();
        });
    EXPECT_EQ(fn->ToString()->StringView(), std::string_view{"[[function]]"});
  }

  EXPECT_EQ(Undefined()->ToString()->StringView(), "undefined");
  EXPECT_EQ(Null()->ToString()->StringView(), "null");
}

TEST_F(ValueTest, ArrayAccessor) {
  auto array = Array::New(5);
  EXPECT_EQ(array->Length(), 5);
  for (int i = 0; i < array->Length(); i++) {
    EXPECT_EQ(array->Index(i), Undefined());
    EXPECT_TRUE(array->Index(i)->Equals(Undefined()));
    array->Set(i, array);
  }
  for (int i = 0; i < array->Length(); i++) {
    EXPECT_EQ(array->Index(i), array);
    EXPECT_TRUE(array->Index(i)->Equals(array));
  }
  array->Push(Number::New(2));
  EXPECT_EQ(array->Length(), 6);
  EXPECT_TRUE(array->Index(5)->IsNumber());
  EXPECT_EQ(array->Index(5)->ToNumber()->Int32(), 2);

  EXPECT_EQ(array->Index(0), array);
  ASSERT_TRUE(array->Index(0)->Equals(array));

  EXPECT_EQ(Handle<Array>::Cast(array->Index(0))->Index(0), array);

  for (int i = 0; i < array->Length() - 1; i++) {
    EXPECT_EQ(array->Index(i), array);
    EXPECT_TRUE(array->Index(i)->Equals(array));
  }
}

TEST_F(ValueTest, FunctionCall) {
  std::string_view params[2]{"param3", "param4"};
  auto fn = Function::New(
      "fn1", 2, params,
      [](Handle<Array> args, Context* context) -> Handle<Value> {
        EXPECT_EQ(args->Length(), 2);
        EXPECT_TRUE(context->Resolve("arguments_")->IsArray());
        EXPECT_EQ(Handle<Array>(context->Resolve("arguments_"))->Index(0),
                  context->Resolve("param3"));
        EXPECT_EQ(Handle<Array>(context->Resolve("arguments_"))->Index(1),
                  context->Resolve("param4"));
        auto result =
            Handle<Number>::Cast(context->Resolve("param3"))->Int32() +
            Handle<Number>::Cast(context->Resolve("param4"))->Int32();
        return Number::New(result);
      });
  auto args = Array::New(2);
  args->Set(0, Number::New(5));
  args->Set(1, Number::New(7));
  auto result = fn->Call(Handle<Value>::Empty(), args, Kipper::GlobalContext());
  EXPECT_EQ(Handle<Number>::Cast(result)->Int32(), 12);
}
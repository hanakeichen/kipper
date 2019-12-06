#pragma once

namespace kipper {
namespace internal {

template <typename T, size_t N = std::numeric_limits<T>::digits10 + 3>
std::array<char, N> IntToString(T value);

template <class T>
T StringToInt(std::string_view str);

double StringToDouble(std::string_view str);

int32_t DoubleToInt32(double value);

int64_t DoubleToInt64(double value);

}  // namespace internal
}  // namespace kipper

#include "conversion-inl.hh"
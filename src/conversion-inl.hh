#pragma once

#include <array>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <string_view>
#include "value.hh"

namespace kipper {
namespace internal {

template <typename T, size_t N>
inline std::array<char, N> IntToString(T value) {
  std::array<char, N> result;
  if (auto [p, ec] =
          std::to_chars(result.data(), result.data() + result.size(), value);
      ec == std::errc()) {
    result[p - result.data()] = '\0';
    return result;
  }
  result[0] = '\0';
  return result;
}

template <class T>
inline T StringToInt(std::string_view str) {
  T result;
  if (auto [p, ec] =
          std::from_chars(str.data(), str.data() + str.size(), result);
      ec == std::errc()) {
    return result;
  }
  return 0;
}

inline double StringToDouble(std::string_view str) {
  char* end;
  if (double result = std::strtod(str.data(), &end); str.data() != end) {
    return result;
  }
  return Double::kNaN;
}

inline int32_t DoubleToInt32(double value) {
  if (std::isnan(value)) {
    return 0;
  }
  constexpr double int32_max = Int32::kMaxInt32;
  constexpr double int32_min = Int32::kMinInt32;
  if (value >= int32_max) {
    return Int32::kMaxInt32;
  }
  if (value <= int32_min) {
    return Int32::kMinInt32;
  }
  return static_cast<int32_t>(value);
}

inline int64_t DoubleToInt64(double value) {
  if (std::isnan(value)) {
    return 0;
  }
  constexpr double int64_max = HeapNumber::kMaxInt64;
  constexpr double int64_min = HeapNumber::kMinInt64;
  if (value >= int64_max) {
    return HeapNumber::kMaxInt64;
  }
  if (value <= int64_min) {
    return HeapNumber::kMinInt64;
  }
  return static_cast<int64_t>(value);
}

}  // namespace internal
}  // namespace kipper
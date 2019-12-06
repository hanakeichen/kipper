#pragma once

#include "kipper.hh"

namespace kipper {
namespace internal {

inline constexpr int Align(size_t size) {
  return (size + kPointerSize - 1) & ~(kPointerSize - 1);
}

inline uint32_t NextPowerOf2(uint32_t x) {
  x--;
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  return x + 1;
}

}  // namespace internal
}  // namespace kipper
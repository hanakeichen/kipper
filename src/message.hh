#pragma once

#include <fmt/core.h>
#include <string>
#include "location.hh"

namespace kipper {
namespace internal {

class Message {
 public:
  template <typename... Args>
  static void Printf(const char* format, Args&&... args) {
    fmt::print(format, args...);
  }

  template <typename... Args>
  static std::string Format(const char* format, Args&&... args) {
    return fmt::format(format, args...);
  }
};

}  // namespace internal
}  // namespace kipper
#pragma once

#include <cstdio>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include "location.hh"

namespace kipper {
namespace internal {

#define DISABLE_DEFAULT_OP(T)      \
  T() = delete;                    \
  T(const T&) = delete;            \
  T(T&&) = delete;                 \
  T& operator=(const T&) = delete; \
  T& operator=(T&&) = delete;

#ifndef NDEBUG
#define UNREACHABLE()                                              \
  do {                                                             \
    printf("%s:%d: %s", __FILE__, __LINE__, "Unreachable line\n"); \
    std::exit(EXIT_FAILURE);                                       \
  } while (false)
#else
#define UNREACHABLE() ((void)0)
#endif

template <typename T>
using unique_ptr = std::unique_ptr<T>;
using Address = unsigned char*;
using Byte = unsigned char;

constexpr int KB = 1024;
constexpr int MB = 1024 * KB;
constexpr int GB = 1024 * MB;

constexpr int kPointerSize = sizeof(void*);
constexpr int kByteBits = 8;

class Interpreter;
class Object;

using ObjectHandleCallback = std::function<void(Object**)>;

class AllStatic {
 public:
  AllStatic() { UNREACHABLE(); }

  ~AllStatic() { UNREACHABLE(); }
};

class Kipper : public AllStatic {
 public:
  static void Initialize();

  static bool IsInitialized() { return initialized_; }

  static Interpreter* interpreter() { return interpreter_; }

 private:
  static bool initialized_;
  static Interpreter* interpreter_;
};

class KError : public std::runtime_error {
 public:
  KError(const std::string& what) : std::runtime_error{what} {}

  KError(const char* what) : std::runtime_error{what} {}

  virtual ~KError() {}
};

class KSError : public KError {
 public:
  KSError(const Location& loc, const std::string& msg)
      : KSError{loc, msg.data()} {}

  KSError(const Location& loc, const char* msg);

  virtual ~KSError() = default;

  virtual const char* what() const noexcept override { return what_.data(); }

 private:
  std::string what_;
};

}  // namespace internal
}  // namespace kipper
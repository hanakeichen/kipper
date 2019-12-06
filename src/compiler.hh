#pragma once

#include <string>
#include "handle.hh"
#include "kipper.hh"
#include "location.hh"

namespace kipper {
namespace internal {

class String;
struct Node;

class KSSyntaxError : public KSError {
 public:
  KSSyntaxError(const Location& loc, const char* message)
      : KSError{loc, message} {}

  KSSyntaxError(const Location& loc, const std::string& message)
      : KSError{loc, message} {}
};

class Compiler {
 public:
  static unique_ptr<Node> Compile(std::string_view code,
                                  std::string_view filename);

  static std::string_view GetLocationSourceCode(const Location& loc);
};

}  // namespace internal
}  // namespace kipper
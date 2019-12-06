#pragma once

#include <string>
#include "location.hh"
#include "token.hh"

namespace kipper {
namespace internal {

using CodeChar = char;

class Scanner {
 public:
  void Initialize(std::string_view code, const Location& loc);

  void NextToken();

  Token::Kind Peek() const { return token_buf_.current; }

  std::string_view CurrentLiteral() {
    return &literal_buf_[token_buf_.literal_offset];
  }

  const Location& CurrentLocation() const { return token_buf_.current_loc; }

  bool has_line_terminator() const { return has_line_terminator_; }

 private:
  struct TokenBuffer {
    Token::Kind current;
    Location current_loc;
    size_t literal_offset;
  };

  Token::Kind Scan();

  Token::Kind ScanDigitLiteral();

  void ScanStringLiteral();

  Token::Kind ScanIdentifier();

  void SkipWhitespace();

  void NextChar();

  [[nodiscard]] bool Consume(CodeChar ch);

  bool AcceptLineTerminator();

  CodeChar CurrentChar() { return *code_; }

  static bool IsWhiteSpace(CodeChar ch);

  static bool IsIdStart(CodeChar ch);

  static bool IsDigit(CodeChar ch);

  const CodeChar* code_;
  Location loc_;
  TokenBuffer token_buf_;
  std::string literal_buf_;
  size_t literal_buf_offset_{0};
  bool has_line_terminator_{false};

  friend class LiteralBufferHandler;
};

}  // namespace internal
}  // namespace kipper
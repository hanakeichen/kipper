#include "scanner.hh"
#include <iostream>
#include "compiler.hh"
#include "kipper.hh"
#include "message.hh"

using namespace kipper::internal;

template <typename... Args>
static void ReportError(const Location& loc, const char* format, Args... args);

namespace kipper::internal {
class LiteralBufferHandler {
 public:
  explicit LiteralBufferHandler(Scanner* scanner) : scanner_{scanner} {
    scanner_->literal_buf_offset_ = scanner_->literal_buf_.size();
  }

  ~LiteralBufferHandler() { scanner_->literal_buf_.push_back('\0'); }

  void AddChar(CodeChar ch) { scanner_->literal_buf_.push_back(ch); }

  std::string_view CurrentLiteral() {
    return &scanner_->literal_buf_[scanner_->literal_buf_offset_];
  }

  DISABLE_DEFAULT_OP(LiteralBufferHandler)

 private:
  Scanner* scanner_;
};
}  // namespace kipper::internal

void Scanner::Initialize(std::string_view code, const Location& loc) {
  code_ = code.data();
  loc_ = loc;
  token_buf_.current = Token::UNKNOWN;
  NextToken();
}

void kipper::internal::Scanner::NextToken() {
  assert(token_buf_.current != Token::END);
  token_buf_.current = Scan();
  token_buf_.current_loc = loc_;
  token_buf_.literal_offset = literal_buf_offset_;
}

Token::Kind Scanner::Scan() {
  SkipWhitespace();
  loc_.step();
  switch (CurrentChar()) {
    case '(':
      NextChar();
      return Token::LP;
    case ')':
      NextChar();
      return Token::RP;
    case '{':
      NextChar();
      return Token::LC;
    case '}':
      NextChar();
      return Token::RC;
    case '[':
      NextChar();
      return Token::LBRACKET;
    case ']':
      NextChar();
      return Token::RBRACKET;
    case ';':
      NextChar();
      return Token::SEMI;
    case ',':
      NextChar();
      return Token::COMMA;
    case '.':
      NextChar();
      return Token::DOT;
    case '+':
      NextChar();
      if (Consume('+')) {
        return Token::INC;
      }
      if (Consume('=')) {
        return Token::ADD_ASSIGN;
      }
      return Token::PLUS;
    case '-':
      NextChar();
      if (Consume('-')) {
        return Token::DEC;
      }
      if (Consume('=')) {
        return Token::SUB_ASSIGN;
      }
      return Token::SUB;
    case '*':
      NextChar();
      if (Consume('=')) {
        return Token::MUL_ASSIGN;
      }
      return Token::MUL;
    case '/':
      NextChar();
      if (Consume('=')) {
        return Token::DIV_ASSIGN;
      }
      return Token::DIV;
    case '%':
      NextChar();
      if (Consume('=')) {
        return Token::MOD_ASSIGN;
      }
      return Token::MOD;
    case '=':
      NextChar();
      if (Consume('=')) {
        return Token::EQ;
      }
      return Token::ASSIGN;
    case '?':
      NextChar();
      return Token::QUES;
    case ':':
      NextChar();
      return Token::COLON;
    case '!':
      NextChar();
      if (Consume('=')) {
        return Token::NE;
      }
      return Token::NOT;
    case '>':
      NextChar();
      if (Consume('=')) {
        return Token::GTE;
      }
      return Token::GT;
    case '<':
      NextChar();
      if (Consume('=')) {
        return Token::LTE;
      }
      return Token::LT;
    case '"':
      ScanStringLiteral();
      return Token::STRING_LITERAL;
    case '#':
      return Scan();
    case '\0':
      return Token::END;
    default:
      if (IsDigit(CurrentChar())) {
        return ScanDigitLiteral();
      }
      if (IsIdStart(CurrentChar())) {
        return ScanIdentifier();
      }
      break;
  }
  ReportError(loc_, "unexpected character: {}", CodeChar());
  UNREACHABLE();
  return Token::UNKNOWN;
}

Token::Kind Scanner::ScanDigitLiteral() {
  assert(IsDigit(CurrentChar()));
  LiteralBufferHandler buf_handler{this};
  buf_handler.AddChar(CurrentChar());
  NextChar();
  while (IsDigit(CurrentChar())) {
    buf_handler.AddChar(CurrentChar());
    NextChar();
  }
  if (Consume('.')) {
    buf_handler.AddChar('.');
    while (IsDigit(CurrentChar())) {
      buf_handler.AddChar(CurrentChar());
      NextChar();
    }
    return Token::DOUBLE_LITERAL;
  }
  return Token::INT_LITERAL;
}

void Scanner::ScanStringLiteral() {
  assert(CurrentChar() == '"');
  LiteralBufferHandler buf_handler{this};
  NextChar();
  static constexpr const auto is_str_part = [](CodeChar ch) -> bool {
    return ch != '"' && ch != '\0';
  };
  while (is_str_part(CurrentChar())) {
    buf_handler.AddChar(CurrentChar());
    NextChar();
  }
  if (Consume('"')) {
    return;
  }
  ReportError(loc_, "expect character `\"`, but got `{}`", CodeChar());
}

Token::Kind Scanner::ScanIdentifier() {
  assert(IsIdStart(CurrentChar()));
  LiteralBufferHandler buf_handler{this};
  buf_handler.AddChar(CurrentChar());
  NextChar();
  static constexpr const auto is_id_part = [](CodeChar ch) -> bool {
    return IsIdStart(ch) || IsDigit(ch);
  };
  while (is_id_part(CurrentChar())) {
    buf_handler.AddChar(CurrentChar());
    NextChar();
  }
  if (auto keyword = Token::FindKeyword(buf_handler.CurrentLiteral())) {
    return *keyword;
  }
  return Token::ID;
}

inline void Scanner::SkipWhitespace() {
  has_line_terminator_ = false;
  do {
    while (IsWhiteSpace(CurrentChar())) {
      NextChar();
    }
    if (Consume('#')) {
      while (!AcceptLineTerminator()) {
        NextChar();
      }
      continue;
    }
    if (AcceptLineTerminator()) {
      continue;
    }
    break;
  } while (true);
}

inline void Scanner::NextChar() {
  if (!has_line_terminator_) {
    loc_.columns();
  }
  code_++;
}

inline bool Scanner::Consume(CodeChar ch) {
  if (CurrentChar() == ch) {
    NextChar();
    return true;
  }
  return false;
}

bool Scanner::AcceptLineTerminator() {
  switch (CurrentChar()) {
    case '\r':
      has_line_terminator_ = true;
      loc_.lines();
      NextChar();
      if (CurrentChar() == '\n') {
        NextChar();
      }
      return true;
    case '\n':
      has_line_terminator_ = true;
      loc_.lines();
      NextChar();
      return true;
  }
  return false;
}

inline bool Scanner::IsWhiteSpace(CodeChar ch) {
  return ch == ' ' || ch == '\t';
}

inline bool Scanner::IsIdStart(CodeChar ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '$' ||
         ch == '_';
}

inline bool Scanner::IsDigit(CodeChar ch) { return ch >= '0' && ch <= '9'; }

template <typename... Args>
static void ReportError(const Location& loc, const char* format, Args... args) {
  throw KSSyntaxError{loc, Message::Format(format, args...)};
}
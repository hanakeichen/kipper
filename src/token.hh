#pragma once

#include <optional>
#include <string_view>

namespace kipper {
namespace internal {

// T: token K: keyword
#define TOKEN_LIST(T, K)              \
  T(LP, "(")                          \
  T(RP, ")")                          \
  T(LC, "{")                          \
  T(RC, "}")                          \
  T(LBRACKET, "[")                    \
  T(RBRACKET, "]")                    \
  T(SEMI, ";")                        \
  T(COMMA, ",")                       \
  T(DOT, ".")                         \
                                      \
  T(PLUS, "+")                        \
  T(SUB, "-")                         \
  T(MUL, "*")                         \
  T(DIV, "/")                         \
  T(MOD, "%")                         \
  T(INC, "++")                        \
  T(DEC, "--")                        \
  T(ASSIGN, "=")                      \
  T(ADD_ASSIGN, "+=")                 \
  T(SUB_ASSIGN, "-=")                 \
  T(MUL_ASSIGN, "*=")                 \
  T(DIV_ASSIGN, "/=")                 \
  T(MOD_ASSIGN, "%=")                 \
                                      \
  T(LOGIC_OR, "||")                   \
  T(LOGIC_AND, "&&")                  \
  T(EQ, "==")                         \
  T(NE, "!=")                         \
  T(LT, "<")                          \
  T(GT, ">")                          \
  T(LTE, "<=")                        \
  T(GTE, ">=")                        \
                                      \
  T(QUES, "?")                        \
  T(COLON, ":")                       \
  T(NOT, "!")                         \
                                      \
  T(ID, "identifier")                 \
  T(INT_LITERAL, "int literal")       \
  T(DOUBLE_LITERAL, "double literal") \
  T(STRING_LITERAL, "string literal") \
                                      \
  K(FUNCTION, "function")             \
  K(IF, "if")                         \
  K(ELSIF, "elsif")                   \
  K(ELSE, "else")                     \
  K(WHILE, "while")                   \
  K(FOR, "for")                       \
  K(RETURN, "return")                 \
  K(BREAK, "break")                   \
  K(CONTINUE, "continue")             \
  K(TRUE, "true")                     \
  K(FALSE, "false")                   \
  K(UNDEFINED, "undefined")           \
                                      \
  T(END, "end of file")               \
  T(UNKNOWN, "unknown")

class Token {
 public:
  enum Kind {
#define TOKEN_DEF(kind, desc) kind,
    TOKEN_LIST(TOKEN_DEF, TOKEN_DEF)
#undef TOKEN_DEF
        SIZE
  };

  static const char* GetTokenDesc(Kind kind);

  static std::optional<Token::Kind> FindKeyword(std::string_view id);

  static bool IsAssignmentOperator(Token::Kind kind) {
    return kind >= Token::ASSIGN && kind <= Token::MOD_ASSIGN;
  }

  static bool IsBinaryOperator(Token::Kind kind) {
    return (kind >= Token::PLUS && kind <= Token::MOD) ||
           (kind >= Token::LOGIC_OR && kind <= Token::GTE);
  }
};

}  // namespace internal
}  // namespace kipper
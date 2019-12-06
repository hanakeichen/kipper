#include "token.hh"
#include <unordered_map>

using namespace kipper::internal;

static const char* token_descs[Token::SIZE] = {
#define TOKEN_DESC(kind, desc) desc,
    TOKEN_LIST(TOKEN_DESC, TOKEN_DESC)
#undef TOKEN_DESC
};

static const std::unordered_map<std::string_view, Token::Kind> keywords = {
#define TOKEN_IGNORE(kind, desc)
#define TOKEN_KEYWORD(kind, desc) \
  {desc, Token::kind},
    TOKEN_LIST(TOKEN_IGNORE, TOKEN_KEYWORD)
#undef TOKEN_KEYWORD
#undef TOKEN_IGNORE
};

const char* Token::GetTokenDesc(Token::Kind kind) {
  return token_descs[kind];
}

std::optional<Token::Kind> Token::FindKeyword(std::string_view id) {
  auto find = keywords.find(id);
  if (find != keywords.end()) {
    return find->second;
  }
  return std::nullopt;
}
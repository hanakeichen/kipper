#include "parser.hh"
#include "compiler.hh"
#include "message.hh"
#include "value.hh"

using namespace kipper::internal;

namespace kipper::internal {

class BreakableScopeHandler {
 public:
  explicit BreakableScopeHandler(Parser* parser) : parser_{parser} {
    parser_->is_breakable_scope_ = true;
  }

  ~BreakableScopeHandler() { parser_->is_breakable_scope_ = false; }

  DISABLE_DEFAULT_OP(BreakableScopeHandler)
 private:
  Parser* parser_;
};

class FunctionScopeHandler {
 public:
  explicit FunctionScopeHandler(Parser* parser) : parser_{parser} {
    parser->is_fn_scope_ = true;
  }

  ~FunctionScopeHandler() { parser_->is_fn_scope_ = false; }

  DISABLE_DEFAULT_OP(FunctionScopeHandler)
 private:
  Parser* parser_;
};

}  // namespace kipper::internal

template <typename... Args>
void ReportError(const Location& loc, const char* format, Args... args) {
  throw KSSyntaxError{loc, Message::Format(format, args...)};
}

Parser::Parser(std::string_view code, const Location& loc)
    : code_{code}, loc_{loc} {}

Node::Ptr Parser::Parse() {
  scanner_.Initialize(code_, loc_);
  auto unit = CreateNode<TranslationUnit>(scanner_.CurrentLocation());
  while (!Look(Token::END)) {
    if (Look(Token::FUNCTION)) {
      unit->fn_decls.push_back(ParseFunctionDecl());
    } else {
      unit->stmts.push_back(ParseStatement());
    }
  }
  return unit;
}

FunctionDecl::Ptr Parser::ParseFunctionDecl() {
  assert(Look(Token::FUNCTION));
  auto result = CreateNode<FunctionDecl>(scanner_.CurrentLocation());
  Next();
  Expect(Token::ID);
  result->name = Handle{String::NewSymbol(scanner_.CurrentLiteral())};
  Expect(Token::LP);
  if (!Look(Token::RP)) {
    FunctionDecl::Params params{};
    do {
      params.push_back(Identifier::Ptr{
          static_cast<Identifier*>(ParseIdentifierName().release())});
    } while (Accept(Token::COMMA));
    result->params = std::move(params);
  }
  Expect(Token::RP);
  Expect(Token::LC);
  FunctionScopeHandler fn_scope{this};
  FunctionDecl::Body body{};
  while (!Look(Token::RC)) {
    body.push_back(ParseStatement());
  }
  result->loc += scanner_.CurrentLocation();
  result->body = std::move(body);
  Expect(Token::RC);
  return result;
}

Statement::Ptr Parser::ParseStatement() {
  switch (Peek()) {
    case Token::LC:
      return ParseBlockStatement();
    case Token::IF:
      return ParseIfStatement();
    case Token::WHILE:
      return ParseWhileStatement();
    case Token::FOR:
      return ParseForStatement();
    case Token::RETURN:
      return ParseReturnStatement();
    case Token::BREAK:
      return ParseBreakStatement();
    case Token::CONTINUE:
      return ParseContinueStatement();
    default:
      return ParseExpressionStatement();
  }
  UNREACHABLE();
}

Statement::Ptr Parser::ParseBlockStatement() {
  assert(Look(Token::LC));
  auto result = CreateNode<BlockStatement>(scanner_.CurrentLocation());
  Expect(Token::LC);
  BlockStatement::Statements stmts;
  while (!Look(Token::RC)) {
    stmts.push_back(ParseStatement());
  }
  result->loc += scanner_.CurrentLocation();
  Expect(Token::RC);
  result->stmts = std::move(stmts);
  return result;
}

Statement::Ptr Parser::ParseIfStatement() {
  assert(Look(Token::IF));
  auto if_stmt = CreateNode<IfStatement>(scanner_.CurrentLocation());
  Next();
  Expect(Token::LP);
  if_stmt->condition = ParseExpression();
  Expect(Token::RP);
  auto then = ParseStatement();
  if_stmt->loc += then->loc;
  if_stmt->then_stmt = std::move(then);
  if (Accept(Token::ELSE)) {
    if_stmt->else_stmt = ParseStatement();
    if_stmt->loc += if_stmt->else_stmt->loc;
  }
  return if_stmt;
}

Statement::Ptr Parser::ParseWhileStatement() {
  assert(Look(Token::WHILE));
  auto loc = scanner_.CurrentLocation();
  Next();
  Expect(Token::LP);
  auto condition = ParseExpression();
  Expect(Token::RP);
  BreakableScopeHandler breakable_scope{this};
  auto loop_stmt = ParseStatement();
  return CreateNode<WhileStatement>(loc + loop_stmt->loc, std::move(condition),
                                    std::move(loop_stmt));
}

Statement::Ptr Parser::ParseForStatement() {
  assert(Look(Token::FOR));
  auto for_stmt = CreateNode<ForStatement>(scanner_.CurrentLocation());
  Next();
  Expect(Token::LP);
  if (!Look(Token::SEMI)) {
    for_stmt->init = ParseExpression();
  }
  Expect(Token::SEMI);
  if (!Look(Token::SEMI)) {
    for_stmt->condition = ParseExpression();
  }
  Expect(Token::SEMI);
  if (!Look(Token::RP)) {
    for_stmt->update = ParseExpression();
  }
  Expect(Token::RP);
  BreakableScopeHandler breakable_scope{this};
  for_stmt->loop_stmt = ParseStatement();
  for_stmt->loc = for_stmt->loop_stmt->loc;
  return for_stmt;
}

Statement::Ptr Parser::ParseReturnStatement() {
  assert(Look(Token::RETURN));
  auto result = CreateNode<ReturnStatement>(scanner_.CurrentLocation());
  Next();
  if (scanner_.has_line_terminator() || Look(Token::END)) {
    return result;
  }
  result->value = ParseExpression();
  result->loc += result->value->loc;
  ExpectEnd();
  if (!is_fn_scope_) {
    ReportError(result->loc, "syntax error: illegal return statement");
  }
  return result;
}

Statement::Ptr Parser::ParseBreakStatement() {
  assert(Look(Token::BREAK));
  auto result = CreateNode<BreakStatement>(scanner_.CurrentLocation());
  Next();
  ExpectEnd();
  if (!is_breakable_scope_) {
    ReportError(result->loc, "syntax error: illegal break statement");
  }
  return result;
}

Statement::Ptr Parser::ParseContinueStatement() {
  assert(Look(Token::CONTINUE));
  auto result = CreateNode<ContinueStatement>(scanner_.CurrentLocation());
  Next();
  ExpectEnd();
  if (!is_breakable_scope_) {
    ReportError(result->loc, "syntax error: illegal continue statement");
  }
  return result;
}

Statement::Ptr Parser::ParseExpressionStatement() {
  auto expr = ParseExpression();
  auto expr_stmt = CreateNode<ExpressionStatement>(expr->loc);
  expr_stmt->expr = std::move(expr);
  expr_stmt->loc = expr_stmt->expr->loc;
  ExpectEnd();
  return expr_stmt;
}

Expression::Ptr Parser::ParseExpression() { return ParseAssignment(); }

Expression::Ptr Parser::ParseAssignment() {
  auto conditional = ParseConditionalExpression();
  if (conditional->IsLeftHandSideExpression() &&
      Token::IsAssignmentOperator(Peek())) {
    auto op = Peek();
    Next();
    auto value = ParseAssignment();
    auto assignment = CreateNode<Assignment>(conditional->loc + value->loc);
    assignment->target = std::move(conditional);
    assignment->op = op;
    assignment->value = std::move(value);
    return assignment;
  }
  return conditional;
}

Expression::Ptr Parser::ParseConditionalExpression() {
  auto logic_or = ParseLogicOrExpression();
  if (Accept(Token::QUES)) {
    auto then = ParseAssignment();
    Expect(Token::COLON);
    auto else_expr = ParseAssignment();
    auto conditional =
        CreateNode<ConditionalExpression>(logic_or->loc + else_expr->loc);
    conditional->condition = std::move(logic_or);
    conditional->then_expr = std::move(then);
    conditional->else_expr = std::move(else_expr);
    return conditional;
  }
  return logic_or;
}

Expression::Ptr Parser::ParseLogicOrExpression() {
  auto result = ParseLogicAndExpression();
  while (Accept(Token::LOGIC_OR)) {
    auto right = ParseLogicAndExpression();
    auto loc = result->loc + right->loc;
    result = CreateNode<BinaryExpression>(loc, std::move(result),
                                          std::move(right), Token::LOGIC_OR);
  }
  return result;
}

Expression::Ptr Parser::ParseLogicAndExpression() {
  auto result = ParseEqualityExpression();
  while (Accept(Token::LOGIC_AND)) {
    auto right = ParseEqualityExpression();
    auto loc = result->loc + right->loc;
    result = CreateNode<BinaryExpression>(loc, std::move(result),
                                          std::move(right), Token::LOGIC_AND);
  }
  return result;
}

Expression::Ptr Parser::ParseEqualityExpression() {
  auto result = ParseRelationalExpression();
  while (Look(Token::EQ) || Look(Token::NE)) {
    auto op = Peek();
    Next();
    auto right = ParseRelationalExpression();
    auto loc = result->loc + right->loc;
    result = CreateNode<BinaryExpression>(loc, std::move(result),
                                          std::move(right), op);
  }
  return result;
}

Expression::Ptr Parser::ParseRelationalExpression() {
  auto result = ParseAdditiveExpression();
  while (Look(Token::LT) || Look(Token::GT) || Look(Token::LTE) ||
         Look(Token::GTE)) {
    auto op = Peek();
    Next();
    auto right = ParseAdditiveExpression();
    auto loc = result->loc + right->loc;
    result = CreateNode<BinaryExpression>(loc, std::move(result),
                                          std::move(right), op);
  }
  return result;
}

Expression::Ptr Parser::ParseAdditiveExpression() {
  auto result = ParseMultiplicativeExpression();
  while (Look(Token::PLUS) || Look(Token::SUB)) {
    auto op = Peek();
    Next();
    auto right = ParseMultiplicativeExpression();
    auto loc = result->loc + right->loc;
    result = CreateNode<BinaryExpression>(loc, std::move(result),
                                          std::move(right), op);
  }
  return result;
}

Expression::Ptr Parser::ParseMultiplicativeExpression() {
  auto result = ParseUnaryExpression();
  while (Look(Token::MUL) || Look(Token::DIV) || Look(Token::MOD)) {
    auto op = Peek();
    Next();
    auto right = ParseUnaryExpression();
    auto loc = result->loc + right->loc;
    result = CreateNode<BinaryExpression>(loc, std::move(result),
                                          std::move(right), op);
  }
  return result;
}

Expression::Ptr Parser::ParseUnaryExpression() {
  auto op = Peek();
  switch (op) {
    case Token::INC:
    case Token::DEC:
    case Token::PLUS:
    case Token::SUB:
    case Token::NOT:
      break;
    default:
      return ParsePostfixExpression();
  }
  auto result = CreateNode<UnaryExpression>(scanner_.CurrentLocation());
  Next();
  result->target = ParseUnaryExpression();
  result->op = op;
  result->loc += result->target->loc;
  return result;
}

Expression::Ptr Parser::ParsePostfixExpression() {
  auto result = ParseLeftHandSideExpression();
  if (Look(Token::INC) || Look(Token::DEC)) {
    auto op = Peek();
    auto loc = result->loc + scanner_.CurrentLocation();
    result = CreateNode<PostfixExpression>(loc, std::move(result), op);
    Next();
  }
  return result;
}

Expression::Ptr Parser::ParseLeftHandSideExpression() {
  return ParseCallExpression();
}

Expression::Ptr Parser::ParseCallExpression() {
  auto result = ParseMemberExpression();
  while (true) {
    if (Accept(Token::LP)) {
      if (Look(Token::RP)) {
        auto loc = result->loc + scanner_.CurrentLocation();
        result = CreateNode<FunctionCall>(loc, std::move(result));
        Next();
        continue;
      }
      FunctionCall::Args args;
      do {
        args.push_back(ParseAssignment());
      } while (Accept(Token::COMMA));
      auto loc = result->loc + scanner_.CurrentLocation();
      result =
          CreateNode<FunctionCall>(loc, std::move(result), std::move(args));
      Expect(Token::RP);
      continue;
    }
    if (Accept(Token::LBRACKET)) {
      auto member = ParseExpression();
      Expect(Token::RBRACKET);
      auto loc = result->loc + member->loc;
      result = CreateNode<MemberAccess>(loc, std::move(result),
                                        std::move(member), MemberAccess::KEYED);
      continue;
    }
    if (Accept(Token::DOT)) {
      auto member = ParseIdentifierName();
      auto loc = result->loc + member->loc;
      result = CreateNode<MemberAccess>(
          loc, std::move(result), std::move(member), MemberAccess::DOTTED);
      continue;
    }
    break;
  }
  return result;
}

Expression::Ptr Parser::ParseMemberExpression() {
  auto result = ParsePrimaryExpression();
  while (true) {
    if (Accept(Token::LBRACKET)) {
      auto member = ParseExpression();
      auto loc = result->loc + scanner_.CurrentLocation();
      result = CreateNode<MemberAccess>(loc, std::move(result),
                                        std::move(member), MemberAccess::KEYED);
      Expect(Token::RBRACKET);
      continue;
    }
    if (Accept(Token::DOT)) {
      auto member = ParseIdentifierName();
      auto loc = result->loc + member->loc;
      result = CreateNode<MemberAccess>(
          loc, std::move(result), std::move(member), MemberAccess::DOTTED);
    }
    break;
  }
  return result;
}

Expression::Ptr Parser::ParsePrimaryExpression() {
  switch (Peek()) {
    case Token::ID:
      return ParseIdentifier();
    case Token::TRUE:
    case Token::FALSE: {
      auto value = Constant::BooleanHandle(Peek() == Token::TRUE);
      auto result =
          CreateNode<BooleanLiteral>(scanner_.CurrentLocation(), value);
      Next();
      return result;
    }
    case Token::INT_LITERAL: {
      auto value = Handle{Int32::Make(atoi(scanner_.CurrentLiteral().data()))};
      auto result = CreateNode<IntLiteral>(scanner_.CurrentLocation(), value);
      Next();
      return result;
    }
    case Token::DOUBLE_LITERAL: {
      auto value = Handle{Double::Make(atoi(scanner_.CurrentLiteral().data()))};
      auto result =
          CreateNode<DoubleLiteral>(scanner_.CurrentLocation(), value);
      Next();
      return result;
    }
    case Token::STRING_LITERAL:
      return ParseStringLiteral();
    case Token::LBRACKET:
      return ParseArrayLiteral();
    case Token::LC:
      return ParseObjectLiteral();
    case Token::LP: {
      Next();
      auto result = ParseExpression();
      Expect(Token::RP);
      return result;
    }
    case Token::UNDEFINED: {
      auto result = CreateNode<UndefinedLiteral>(scanner_.CurrentLocation());
      Next();
      return result;
    }
    default:
      break;
  }
  ReportError(scanner_.CurrentLocation(), "syntax error: unexpected token `{}`",
              Token::GetTokenDesc(Peek()));
  UNREACHABLE();
  return nullptr;
}

Expression::Ptr Parser::ParseArrayLiteral() {
  assert(Look(Token::LBRACKET));
  auto start_loc = scanner_.CurrentLocation();
  Next();
  if (Look(Token::RBRACKET)) {
    auto result =
        CreateNode<ArrayLiteral>(start_loc + scanner_.CurrentLocation());
    Next();
    return result;
  }
  ArrayLiteral::Elements elements;
  do {
    elements.push_back(ParseAssignment());
  } while (Accept(Token::COMMA));
  auto array_literal =
      CreateNode<ArrayLiteral>(start_loc + scanner_.CurrentLocation());
  Expect(Token::RBRACKET);
  array_literal->elements = std::move(elements);
  return array_literal;
}

Expression::Ptr Parser::ParseObjectLiteral() {
  assert(Look(Token::LC));
  auto start_loc = scanner_.CurrentLocation();
  Next();
  if (Look(Token::RC)) {
    auto result =
        CreateNode<ObjectLiteral>(start_loc + scanner_.CurrentLocation());
    Next();
    return result;
  }
  ObjectLiteral::Properties properties;
  do {
    auto property = CreateNode<PropertyAssignment>(scanner_.CurrentLocation());
    property->name = ParsePropertyName();
    Expect(Token::COLON);
    property->value = ParseAssignment();
    property->loc += property->value->loc;
    properties.push_back(std::move(property));
  } while (Accept(Token::COMMA));
  auto result = CreateNode<ObjectLiteral>(
      start_loc + scanner_.CurrentLocation(), std::move(properties));
  Expect(Token::RC);
  return result;
}

Node::Ptr Parser::ParsePropertyName() {
  switch (Peek()) {
    case Token::ID:
      return ParseIdentifierName();
    case Token::STRING_LITERAL:
      return ParseStringLiteral();
    default:
      break;
  }
  ReportError(scanner_.CurrentLocation(),
              "syntax error: unexpected character `{}`",
              Token::GetTokenDesc(Peek()));
  UNREACHABLE();
  return nullptr;
}

Expression::Ptr Parser::ParseStringLiteral() {
  assert(Look(Token::STRING_LITERAL));
  auto literal = Handle{String::New(scanner_.CurrentLiteral().data(), TENURED)};
  auto str = CreateNode<StringLiteral>(scanner_.CurrentLocation(), literal);
  Next();
  return str;
}

Expression::Ptr Parser::ParseIdentifier() {
  assert(Look(Token::ID));
  auto symbol = Handle{String::NewSymbol(scanner_.CurrentLiteral())};
  auto id = CreateNode<Identifier>(scanner_.CurrentLocation(), symbol);
  Next();
  return id;
}

Node::Ptr Parser::ParseIdentifierName() {
  assert(Look(Token::ID));
  auto symbol = Handle{String::NewSymbol(scanner_.CurrentLiteral())};
  auto id_name = CreateNode<IdentifierName>(scanner_.CurrentLocation(), symbol);
  Next();
  return id_name;
}

inline void Parser::Expect(Token::Kind kind) {
  if (Accept(kind)) {
    return;
  }
  ReportError(scanner_.CurrentLocation(), "expected `{}`, but got `{}`",
              Token::GetTokenDesc(kind), Token::GetTokenDesc(Peek()));
}

inline void Parser::ExpectEnd() {
  if (scanner_.has_line_terminator() || Accept(Token::SEMI) ||
      Look(Token::END)) {
    return;
  }
  ReportError(scanner_.CurrentLocation(), "expected `;`, but got {}",
              Token::GetTokenDesc(Peek()));
}

inline bool Parser::Accept(Token::Kind kind) {
  if (Look(kind)) {
    Next();
    return true;
  }
  return false;
}
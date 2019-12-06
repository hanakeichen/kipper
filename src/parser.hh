#pragma once

#include <string_view>
#include "ast.hh"
#include "scanner.hh"

namespace kipper {
namespace internal {

class Parser {
 public:
  Parser(std::string_view code, const Location& loc);

  Node::Ptr Parse();

 private:
  FunctionDecl::Ptr ParseFunctionDecl();

  Statement::Ptr ParseStatement();

  Statement::Ptr ParseBlockStatement();

  Statement::Ptr ParseIfStatement();

  Statement::Ptr ParseWhileStatement();

  Statement::Ptr ParseForStatement();

  Statement::Ptr ParseReturnStatement();

  Statement::Ptr ParseBreakStatement();

  Statement::Ptr ParseContinueStatement();

  Statement::Ptr ParseExpressionStatement();

  Expression::Ptr ParseExpression();

  Expression::Ptr ParseAssignment();

  Expression::Ptr ParseConditionalExpression();

  Expression::Ptr ParseLogicOrExpression();

  Expression::Ptr ParseLogicAndExpression();

  Expression::Ptr ParseEqualityExpression();

  Expression::Ptr ParseRelationalExpression();

  Expression::Ptr ParseAdditiveExpression();

  Expression::Ptr ParseMultiplicativeExpression();

  Expression::Ptr ParseUnaryExpression();

  Expression::Ptr ParsePostfixExpression();

  Expression::Ptr ParseLeftHandSideExpression();

  Expression::Ptr ParseCallExpression();

  Expression::Ptr ParseMemberExpression();

  Expression::Ptr ParsePrimaryExpression();

  Expression::Ptr ParseArrayLiteral();

  Expression::Ptr ParseObjectLiteral();

  Node::Ptr ParsePropertyName();

  Expression::Ptr ParseStringLiteral();

  Expression::Ptr ParseIdentifier();

  Node::Ptr ParseIdentifierName();

  Token::Kind Peek() const { return scanner_.Peek(); }

  void Next() { scanner_.NextToken(); }

  void Expect(Token::Kind kind);

  void ExpectEnd();

  bool Look(Token::Kind kind) const { return Peek() == kind; }

  [[nodiscard]] bool Accept(Token::Kind kind);

  std::string_view code_;
  Location loc_;
  Scanner scanner_;
  bool is_breakable_scope_{false};
  bool is_fn_scope_{false};

  friend class BreakableScopeHandler;
  friend class FunctionScopeHandler;
};

}  // namespace internal
}  // namespace kipper
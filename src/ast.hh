#pragma once

#include <memory>
#include <vector>

#include "completion.hh"
#include "handle.hh"
#include "kipper.hh"
#include "location.hh"
#include "token.hh"

namespace kipper {
namespace internal {

struct Literal;
struct Identifier;
struct BlockStatement;
struct TranslationUnit;
struct MemberAccess;
struct FunctionDecl;
class Execution;
class Statement;
class NodeVisitor;

#define VISIT_NODES(K)     \
  K(TranslationUnit)       \
  K(BlockStatement)        \
  K(IfStatement)           \
  K(WhileStatement)        \
  K(ForStatement)          \
  K(ReturnStatement)       \
  K(BreakStatement)        \
  K(ContinueStatement)     \
  K(ExpressionStatement)   \
  K(Assignment)            \
  K(ConditionalExpression) \
  K(BinaryExpression)      \
  K(UnaryExpression)       \
  K(PostfixExpression)     \
  K(MemberAccess)          \
  K(Identifier)            \
  K(IdentifierName)        \
  K(IntLiteral)            \
  K(DoubleLiteral)         \
  K(StringLiteral)         \
  K(BooleanLiteral)        \
  K(ArrayLiteral)          \
  K(ObjectLiteral)         \
  K(UndefinedLiteral)      \
  K(FunctionCall)          \
  K(FunctionDecl)

struct Node {
 public:
  using Ptr = unique_ptr<Node>;

  virtual ~Node() {}

  virtual TranslationUnit *AsTranslationUnit() { return nullptr; }

  virtual Statement *AsStatement() { return nullptr; }

  virtual BlockStatement *AsBlockStatement() { return nullptr; }

  virtual FunctionDecl *AsFunctionDecl() { return nullptr; }

  virtual Handle<Object> Evaluate(Execution &exec) = 0;

  virtual void Accept(NodeVisitor *visitor) = 0;

  Location loc{"anon"};
};

class Statement : public Node {
 public:
  using Ptr = unique_ptr<Statement>;

  virtual ~Statement() = default;

  virtual Completion Execute(Execution &exec) = 0;

  Statement *AsStatement() override final { return this; }

  virtual bool IsBreakableStatement() const { return false; }

 private:
  Handle<Object> Evaluate(Execution &exec) override final;
};

struct BreakableStatement : public Statement {
  virtual ~BreakableStatement() = default;

  bool IsBreakableStatement() const override final { return true; }
};

struct Expression : public Node {
  using Ptr = unique_ptr<Expression>;

  virtual ~Expression() = default;

  virtual Literal *AsLiteral() { return nullptr; }

  virtual Identifier *AsIdentifier() { return nullptr; }

  virtual MemberAccess *AsMemberAccess() { return nullptr; }

  virtual bool IsLeftHandSideExpression() const { return false; }
};

struct TranslationUnit final : public Node {
  using Ptr = unique_ptr<TranslationUnit>;

  TranslationUnit *AsTranslationUnit() override { return this; }

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  std::vector<Node::Ptr> stmts;
  std::vector<Node::Ptr> fn_decls;
};

/////////////////////////statements ////////////////////

struct BlockStatement final : public Statement {
  using Statements = std::vector<Statement::Ptr>;

  BlockStatement *AsBlockStatement() override { return this; }

  Completion Execute(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Statements stmts;
};

struct IfStatement final : public Statement {
  using Ptr = unique_ptr<IfStatement>;

  Completion Execute(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Expression::Ptr condition;
  Statement::Ptr then_stmt;
  Statement::Ptr else_stmt;
};

struct WhileStatement final : public BreakableStatement {
  WhileStatement(Expression::Ptr _condition, Statement::Ptr _loop_stmt)
      : condition{std::move(_condition)}, loop_stmt{std::move(_loop_stmt)} {}

  Completion Execute(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Expression::Ptr condition;
  Statement::Ptr loop_stmt;
};

struct ForStatement final : public BreakableStatement {
  Completion Execute(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Expression::Ptr init;
  Expression::Ptr condition;
  Expression::Ptr update;
  Statement::Ptr loop_stmt;
};

struct ReturnStatement final : public Statement {
  Completion Execute(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Expression::Ptr value;
};

struct BreakStatement final : public Statement {
  Completion Execute(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;
};

struct ContinueStatement final : public Statement {
  Completion Execute(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;
};

struct ExpressionStatement final : public Statement {
  Completion Execute(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Expression::Ptr expr;
};

////////////////// expressions///////////////////////

struct Assignment final : public Expression {
  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Expression::Ptr target;
  Expression::Ptr value;
  Token::Kind op;
};

struct ConditionalExpression final : public Expression {
  using Ptr = unique_ptr<ConditionalExpression>;

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Expression::Ptr condition;
  Expression::Ptr then_expr;
  Expression::Ptr else_expr;
};

struct BinaryExpression final : public Expression {
  BinaryExpression(Expression::Ptr _left, Expression::Ptr _right,
                   Token::Kind _op)
      : left{std::move(_left)}, right{std::move(_right)}, op{_op} {}

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Expression::Ptr left;
  Expression::Ptr right;
  Token::Kind op;
};

struct UnaryExpression final : public Expression {
  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Expression::Ptr target;
  Token::Kind op;
};

struct PostfixExpression final : public Expression {
  PostfixExpression(Expression::Ptr _target, Token::Kind _op)
      : target{std::move(_target)}, op{_op} {}

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Expression::Ptr target;
  Token::Kind op;
};

struct FunctionCall final : public Expression {
  using Args = std::vector<Expression::Ptr>;

  FunctionCall(Expression::Ptr _target) : target{std::move(_target)} {}

  FunctionCall(Expression::Ptr _target, Args _args)
      : target{std::move(_target)}, args{std::move(_args)} {}

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Expression::Ptr target;
  Args args;
};

struct MemberAccess final : public Expression {
  enum Type { KEYED, DOTTED };

  MemberAccess(Expression::Ptr _target, Node::Ptr _member, Type _type)
      : target{std::move(_target)}, member{std::move(_member)}, type{_type} {}

  MemberAccess *AsMemberAccess() override final { return this; }

  bool IsLeftHandSideExpression() const override final { return true; }

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Expression::Ptr target;
  Node::Ptr member;
  Type type;
};

struct Identifier final : public Expression {
  using Ptr = unique_ptr<Identifier>;

  Identifier(Handle<String> _name) : name{_name} {}

  Identifier *AsIdentifier() override { return this; }

  bool IsLeftHandSideExpression() const override final { return true; }

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Handle<String> name;
};

struct IdentifierName : public Node {
  IdentifierName(Handle<String> _name) : name{_name} {}

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Handle<String> name;
};

struct Literal : public Expression {
  virtual ~Literal() {}

  Literal *AsLiteral() override { return this; }
};

class IntLiteral final : public Literal {
 public:
  IntLiteral(Handle<Object> value) : value_{value} {}

  Handle<Object> value() const { return value_; }

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

 private:
  Handle<Object> value_;
};

class DoubleLiteral final : public Literal {
 public:
  DoubleLiteral(Handle<Object> value) : value_{value} {}

  auto value() const { return value_; }

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

 private:
  Handle<Object> value_;
};

class StringLiteral final : public Literal {
 public:
  StringLiteral(Handle<String> value) : value_{value} {}

  Handle<String> value() const { return value_; }

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

 private:
  Handle<String> value_;
};

class BooleanLiteral final : public Literal {
 public:
  BooleanLiteral(Handle<Object> value) : value_{value} {}

  bool value() const { return value_; }

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

 private:
  Handle<Object> value_;
};

class ArrayLiteral final : public Literal {
 public:
  using Ptr = unique_ptr<ArrayLiteral>;
  using Elements = std::vector<Expression::Ptr>;

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Elements elements;
};

struct UndefinedLiteral : public Literal {
  using Ptr = unique_ptr<UndefinedLiteral>;

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;
};

struct PropertyAssignment final : public Node {
  using Ptr = unique_ptr<PropertyAssignment>;

  Node::Ptr name;
  Expression::Ptr value;

 private:
  Handle<Object> Evaluate(Execution &exec) override final {
    UNREACHABLE();
    return Handle<Object>{};
  }

  void Accept(NodeVisitor *visitor) override final { UNREACHABLE(); }
};

struct ObjectLiteral final : public Literal {
  using Ptr = unique_ptr<ObjectLiteral>;
  using Properties = std::vector<PropertyAssignment::Ptr>;

  ObjectLiteral() = default;

  ObjectLiteral(Properties _properties) : properties{std::move(_properties)} {}

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Properties properties;
};

struct FunctionDecl final : public Node {
  using Ptr = unique_ptr<FunctionDecl>;
  using Params = std::vector<IdentifierName::Ptr>;
  using Body = std::vector<Statement::Ptr>;

  FunctionDecl *AsFunctionDecl() override final { return this; }

  Handle<Object> Evaluate(Execution &exec) override final;

  void Accept(NodeVisitor *visitor) override final;

  Handle<String> name;
  Params params;
  Body body;
};

class NodeVisitor {
 public:
  virtual ~NodeVisitor() = default;

#define VISIT_NODE(Node) virtual void Visit##Node(Node *) = 0;
  VISIT_NODES(VISIT_NODE)
#undef VISIT_NODE
};

template <typename T, typename... Args>
inline unique_ptr<T> CreateNode(const Location &loc, Args &&... args) {
  auto result = std::make_unique<T>(std::forward<Args>(args)...);
  result->loc = loc;
  return result;
}

}  // namespace internal
}  // namespace kipper
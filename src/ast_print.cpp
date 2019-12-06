#include "ast_print.hh"

using namespace kipper::internal;

void AstPrinter::VisitTranslationUnit(TranslationUnit* unit) {
  for (auto& fn_decl : unit->fn_decls) {
    fn_decl->Accept(this);
  }
  for (auto& stmt : unit->stmts) {
    stmt->Accept(this);
  }
}

void AstPrinter::VisitBlockStatement(BlockStatement* block) {
  out_ << "{\n";
  for (auto& stmt : block->stmts) {
    stmt->Accept(this);
  }
  out_ << "}\n";
}

void AstPrinter::VisitIfStatement(IfStatement* if_stmt) {
  out_ << "if (";
  if_stmt->condition->Accept(this);
  out_ << ")";
  if_stmt->then_stmt->Accept(this);
  if (if_stmt->else_stmt) {
    out_ << " else ";
    if_stmt->else_stmt->Accept(this);
  }
}

void AstPrinter::VisitWhileStatement(WhileStatement* while_stmt) {
  out_ << "while (";
  while_stmt->condition->Accept(this);
  out_ << ")";
  while_stmt->loop_stmt->Accept(this);
}

void AstPrinter::VisitForStatement(ForStatement* for_stmt) {
  out_ << "for (";
  if (for_stmt->init) {
    for_stmt->init->Accept(this);
  }
  out_ << "; ";
  if (for_stmt->condition) {
    for_stmt->condition->Accept(this);
  }
  out_ << "; ";
  if (for_stmt->update) {
    for_stmt->update->Accept(this);
  }
  out_ << ")";
  for_stmt->loop_stmt->Accept(this);
}

void AstPrinter::VisitReturnStatement(ReturnStatement* return_stmt) {
  out_ << "return ";
  if (return_stmt->value) {
    return_stmt->value->Accept(this);
  }
  out_ << ";\n";
}

void AstPrinter::VisitBreakStatement(BreakStatement* /*unused*/) {
  out_ << "break;\n";
}

void AstPrinter::VisitContinueStatement(ContinueStatement* /*unused*/) {
  out_ << "continue;\n";
}

void AstPrinter::VisitExpressionStatement(ExpressionStatement* expr_stmt) {
  expr_stmt->expr->Accept(this);
  out_ << ";\n";
}

void AstPrinter::VisitAssignment(Assignment* assign) {
  assign->target->Accept(this);
  out_ << " " << Token::GetTokenDesc(assign->op) << " ";
  assign->value->Accept(this);
}

void AstPrinter::VisitConditionalExpression(
    ConditionalExpression* conditional) {
  conditional->condition->Accept(this);
  out_ << " ? ";
  conditional->then_expr->Accept(this);
  out_ << " : ";
  conditional->else_expr->Accept(this);
}

void AstPrinter::VisitBinaryExpression(BinaryExpression* binary_expr) {
  binary_expr->left->Accept(this);
  out_ << " " << Token::GetTokenDesc(binary_expr->op) << " ";
  binary_expr->right->Accept(this);
}

void AstPrinter::VisitUnaryExpression(UnaryExpression* unary_expr) {
  out_ << Token::GetTokenDesc(unary_expr->op);
  unary_expr->target->Accept(this);
}

void AstPrinter::VisitPostfixExpression(PostfixExpression* postfix_expr) {
  postfix_expr->target->Accept(this);
  out_ << Token::GetTokenDesc(postfix_expr->op);
}

void AstPrinter::VisitMemberAccess(MemberAccess* member_access) {
  member_access->target->Accept(this);
  if (member_access->type == MemberAccess::DOTTED) {
    out_ << ".";
    member_access->member->Accept(this);
  } else {
    out_ << "[";
    member_access->member->Accept(this);
    out_ << "]";
  }
}

void AstPrinter::VisitIdentifier(Identifier* id) { out_ << id->name->Value(); }

void AstPrinter::VisitIdentifierName(IdentifierName* id_name) {
  out_ << id_name->name->Value();
}

void AstPrinter::VisitIntLiteral(IntLiteral* int_literal) {
  out_ << int_literal->value();
}

void AstPrinter::VisitDoubleLiteral(DoubleLiteral* double_literal) {
  out_ << double_literal->value();
}

void AstPrinter::VisitStringLiteral(StringLiteral* str_literal) {
  out_ << "\"" << str_literal->value()->Value() << "\"";
}

void AstPrinter::VisitBooleanLiteral(BooleanLiteral* bool_literal) {
  out_ << (bool_literal->value() ? "true" : "false");
}

void AstPrinter::VisitArrayLiteral(ArrayLiteral* arr_literal) {
  out_ << "[";
  for (auto& element : arr_literal->elements) {
    element->Accept(this);
    if (&element != &arr_literal->elements.back()) {
      out_ << ",";
    }
  }
  out_ << "]";
}

void AstPrinter::VisitObjectLiteral(ObjectLiteral* obj_literal) {
  out_ << "{";
  for (auto& prop : obj_literal->properties) {
    prop->name->Accept(this);
    out_ << ": ";
    prop->value->Accept(this);
    if (&prop != &obj_literal->properties.back()) {
      out_ << ", ";
    }
  }
  out_ << "}";
}

void AstPrinter::VisitUndefinedLiteral(UndefinedLiteral* /*unused*/) {
  out_ << "undefined";
}

void AstPrinter::VisitFunctionCall(FunctionCall* fn_call) {
  fn_call->target->Accept(this);
  out_ << "(";
  for (auto& arg : fn_call->args) {
    arg->Accept(this);
    if (&arg != &fn_call->args.back()) {
      out_ << ", ";
    }
  }
  out_ << ")";
}

void AstPrinter::VisitFunctionDecl(FunctionDecl* fn_decl) {
  out_ << "function " << fn_decl->name->Value();
  out_ << "(";
  for (auto& id : fn_decl->params) {
    id->Accept(this);
    if (&id != &fn_decl->params.back()) {
      out_ << ", ";
    }
  }
  out_ << ") ";
  out_ << "{\n";
  for (auto& stmt : fn_decl->body) {
    stmt->Accept(this);
  }
  out_ << "}\n\n";
}
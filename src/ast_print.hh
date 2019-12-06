#pragma once

#include <ostream>
#include "ast.hh"

namespace kipper {
namespace internal {

class AstPrinter final : public NodeVisitor {
 public:
  AstPrinter(std::ostream& out) : out_{out} {}

#define NODE_PRINT(Node) void Visit##Node(Node*) override final;
  VISIT_NODES(NODE_PRINT)
#undef NODE_PRINT

 private:
  std::ostream& out_;
};

}  // namespace internal
}  // namespace kipper
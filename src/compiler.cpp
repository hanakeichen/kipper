#include "compiler.hh"
#include <iostream>
#include <unordered_map>
#include "ast.hh"
#include "ast_print.hh"
#include "kipper.hh"
#include "log.hh"
#include "message.hh"
#include "parser.hh"
#include "scanner.hh"

using namespace kipper::internal;

static std::unordered_map<std::string_view, std::string_view> scripts;

class CodeStreamBuf : public std::streambuf {
 public:
  explicit CodeStreamBuf(std::string_view code) {
    this->setg(const_cast<char*>(code.data()), const_cast<char*>(code.data()),
               const_cast<char*>(code.data() + code.size()));
  }
};

Node::Ptr Compiler::Compile(std::string_view code, std::string_view filename) {
  LOG_DEBUG("KS compiles file: {}", filename);
  if (!filename.empty()) {
    scripts[filename] = code;
  }

  auto loc = Location{filename};
  Parser parser{code, loc};

  Node::Ptr result = parser.Parse();

#if !defined(NDEBUG) && defined(ENABLE_AST_PRINT)
  AstPrinter ast_printer{std::cout};
  result->Accept(&ast_printer);
#endif

  return result;
}

std::string_view Compiler::GetLocationSourceCode(
    const Location& loc) {
  if (loc.begin.filename.empty()) {
    return std::string_view{};
  }
  auto find = scripts.find(loc.begin.filename);
  if (find == scripts.end()) {
    return std::string_view{};
  }
  auto script = find->second;
  auto current_line = 1;
  auto current_column = 1;
  auto source_start = 0;
  auto source_end = 0;
  for (auto ch : script) {
    if (current_line == loc.begin.line && current_column == loc.begin.column) {
      break;
    }
    if (ch != '\n') {
      current_column++;
    } else {
      current_column = 1;
      current_line++;
    }
    source_start++;
  }
  source_end = source_start;
  for (auto ch : script.substr(source_start)) {
    if (current_line == loc.end.line && current_column == loc.end.column) {
      break;
    }
    if (ch != '\n') {
      current_column++;
    } else {
      current_column = 1;
      current_line++;
    }
    source_end++;
  }
  if (source_end >= source_start) {
    return script.substr(source_start, source_end - source_start);
  }
  return std::string_view{};
}

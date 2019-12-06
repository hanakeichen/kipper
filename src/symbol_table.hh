#pragma once

#include <string_view>
#include <unordered_set>

namespace kipper {
namespace internal {

class ObjectVisitor;
class String;

class SymbolKey {
 public:
  virtual ~SymbolKey() {}

  bool operator==(const SymbolKey& that) const;

  virtual std::string_view string() const = 0;

  virtual String* symbol() const = 0;

  virtual String** symbol_handle() = 0;
};

class SymbolTable {
 public:
  void Insert(String* symbol);

  String* Find(std::string_view key);

  const auto begin() const { return table_.begin(); }

  const auto end() const { return table_.end(); }

  void Cleanup();

 private:
  struct SymbolKeyHasher {
    size_t operator()(const SymbolKey* key) const;
  };

  struct SymbolKeyEquals {
    bool operator()(const SymbolKey* obj1, const SymbolKey* obj2) const;
  };

  std::unordered_set<SymbolKey*, SymbolKeyHasher, SymbolKeyEquals> table_;
};

}  // namespace internal
}  // namespace kipper
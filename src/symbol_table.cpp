#include "symbol_table.hh"
#include "value.hh"

using namespace kipper::internal;

class KSStringSymbolKey : public SymbolKey {
 public:
  explicit KSStringSymbolKey(String* symbol) : symbol_{symbol} {}

  std::string_view string() const override { return symbol_->Value(); }

  String* symbol() const override { return symbol_; }

  String** symbol_handle() override { return &symbol_; }

 private:
  String* symbol_;
};

class StringSymbolKey : public SymbolKey {
 public:
  explicit StringSymbolKey(std::string_view value) : value_{value} {}

  std::string_view string() const override { return value_; }

  String* symbol() const override {
    UNREACHABLE();
    return nullptr;
  }

  String** symbol_handle() override {
    UNREACHABLE();
    return nullptr;
  }

 private:
  std::string_view value_;
};

inline bool SymbolKey::operator==(const SymbolKey& that) const {
  return this->string() == that.string();
}

inline size_t SymbolTable::SymbolKeyHasher::operator()(
    const SymbolKey* key) const {
  return std::hash<std::string_view>{}(key->string());
}

inline bool SymbolTable::SymbolKeyEquals::operator()(
    const SymbolKey* obj1, const SymbolKey* obj2) const {
  return obj1->string() == obj2->string();
}

void SymbolTable::Insert(String* symbol) {
  table_.insert(new KSStringSymbolKey{symbol});
}

String* SymbolTable::Find(std::string_view key) {
  StringSymbolKey symbol_key{key};
  auto search = table_.find(&symbol_key);
  if (search != table_.end()) {
    return (*search)->symbol();
  }
  return nullptr;
}

void SymbolTable::Cleanup() {
  for (auto it = table_.begin(); it != table_.end();) {
    if (!(*it)->symbol()->metadata().IsMarked()) {
      auto symbol_key = *it;
      it = table_.erase(it);
      delete symbol_key;
    } else {
      ++it;
    }
  }
}

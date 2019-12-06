#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include "kipper/kipper.hh"

int read_file(std::string_view file, std::string& kscript) {
  std::ifstream istrm{file.data(), std::ios::in | std::ios::ate};
  if (!istrm.is_open()) {
    std::cerr << "failed to open " << file << '\n';
    return 1;
  }
  auto length = istrm.tellg();
  kscript.resize(length);
  istrm.seekg(0, std::ios::beg);
  istrm.read(&kscript[0], length);
  return 0;
}

void register_assert() {
  using namespace kipper;

  auto fn_name = "Assert";
  std::string_view params[1]{"condition"};
  Kipper::GlobalContext()->Push(
      fn_name,
      Function::New(fn_name, params,
                    [](Handle<Array> args, Context* context) -> Handle<Value> {
                      if (!args->Index(0)->ToBoolean()->Value()) {
                        std::cerr << Handle<String>::Cast(
                                         args->Index(args->Length() - 1))
                                         ->StringView()
                                  << "\n";
                        std::exit(-1);
                      }
                      return Undefined();
                    }));
}

int run_script(std::string_view file) {
  std::string kscript;
  if (auto rcode = read_file(file, kscript)) {
    return rcode;
  }

  kipper::Kipper::Configure({16 * 1024 /* 16 KB*/, 3});
  kipper::Kipper::Initialize();
  register_assert();
  try {
    auto script = kipper::Script::Compile(kscript, file);
    script->Run(kipper::Kipper::GlobalContext());
    return 0;
  } catch (const std::exception& e) {
    printf("%s\n", e.what());
  }
  return 1;
}

int main(int argc, char** argv) {
  assert(argc > 1);
  return run_script(argv[1]);
}
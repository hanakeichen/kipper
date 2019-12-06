#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include "kipper/kipper.hh"

void print_usage() { std::cout << "Usage: ks <source file>" << std::endl; }

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

int run_script(std::string_view file) {
  std::string kscript;
  if (auto rcode = read_file(file, kscript)) {
    return rcode;
  }

  kipper::Kipper::Initialize();
  try {
    auto script = kipper::Script::Compile(kscript, file);
    script->Run(kipper::Kipper::GlobalContext());
    return 0;
  } catch (const std::exception& e) {
    fprintf(stderr, "%s\n", e.what());
  }
  return 1;
}

int main(int argc, char** argv) {
  if (argc == 1) {
    print_usage();
    return 1;
  }
  return run_script(argv[1]);
}
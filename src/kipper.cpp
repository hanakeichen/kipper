#include "kipper.hh"
#include <fmt/ostream.h>
#include "compiler.hh"
#include "heap.hh"
#include "interpreter.hh"
#include "message.hh"
#include "runtime.hh"

using namespace kipper::internal;

bool Kipper::initialized_{false};
static Interpreter current_interpreter{};
Interpreter* Kipper::interpreter_{nullptr};

void Kipper::Initialize() {
  if (IsInitialized()) {
    return;
  }
  Heap::Initialize();
  interpreter_ = &current_interpreter;

  Runtime::InstallNative();
  initialized_ = true;
}

KSError::KSError(const Location& loc, const char* msg)
    : KError{msg}, what_{Message::Format("{}: {}", loc, msg)} {}
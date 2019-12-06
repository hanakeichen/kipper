#pragma once

#include "kipper.hh"

namespace kipper {
namespace internal {

class Runtime : public AllStatic {
 public:
  static void InstallNative();

 private:
  static void InstallNativeProperties();

  static void InstallNativeFunctions();

  static bool installed_;
};

}  // namespace internal
}  // namespace kipper
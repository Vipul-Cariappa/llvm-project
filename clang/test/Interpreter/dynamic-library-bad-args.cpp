// RUN: cat %s | clang-repl 2>&1 | FileCheck %s
// RUN: cat %s | clang-repl -Xcc -fparse-functions-ondemand 2>&1 | FileCheck %s
%lib
// CHECK: %lib expects 1 argument: the path to a dynamic library
%quit

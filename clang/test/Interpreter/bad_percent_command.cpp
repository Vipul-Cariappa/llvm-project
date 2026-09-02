// RUN: cat %s | clang-repl 2>&1 | FileCheck %s
// RUN: cat %s | clang-repl -Xcc -fparse-functions-ondemand 2>&1 | FileCheck %s
%foobar
// CHECK: Invalid % command "%foobar", use "%help" to list commands
%quit

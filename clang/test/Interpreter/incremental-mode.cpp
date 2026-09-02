// RUN: clang-repl -Xcc -E
// RUN: clang-repl -Xcc -fparse-functions-ondemand -Xcc -E
// RUN: clang-repl -Xcc -emit-llvm
// RUN: clang-repl -Xcc -fparse-functions-ondemand -Xcc -emit-llvm
// RUN: clang-repl -Xcc -xc
// RUN: clang-repl -Xcc -fparse-functions-ondemand -Xcc -xc
// expected-no-diagnostics

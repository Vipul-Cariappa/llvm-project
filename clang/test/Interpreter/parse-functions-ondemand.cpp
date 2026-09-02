// With -fparse-functions-ondemand a function body is parsed only when the
// function is used, and it is emitted when its body becomes available.
// clang-format off
// RUN: cat %s | clang-repl -Xcc -fparse-functions-ondemand | FileCheck %s
// RUN: cat %s | clang-repl -Xcc -fparse-functions-ondemand -Xcc -O2 | FileCheck %s

extern "C" int printf(const char *, ...);

// An external-linkage function is deferred as well, and parsed when called.
int add(int a, int b) { return a + b; }
auto r1 = printf("add = %d\n", add(2, 3));
// CHECK: add = 5

// The definition is deferred in an earlier partial translation unit than the
// one using it.
static int counter() { static int c = 0; return ++c; }
auto r2 = printf("counter = %d %d\n", counter(), counter());
// CHECK-NEXT: counter = 1 2

// Member function bodies are deferred too.
struct Point { int x, y; Point(int a, int b) : x(a), y(b) {} int sum() const { return x + y; } };
auto r3 = printf("sum = %d\n", Point(3, 4).sum());
// CHECK-NEXT: sum = 7

// Recursive functions are parsed once, on first use.
int fact(int n) { return n <= 1 ? 1 : n * fact(n - 1); }
auto r4 = printf("fact = %d\n", fact(5));
// CHECK-NEXT: fact = 120

// The body of a function that is never used is never parsed, so the errors it
// contains are never diagnosed.
int never_used() { this_function_does_not_exist(); return 0; }

auto r5 = printf("done\n");
// CHECK-NEXT: done

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

// Members of a local class are deferred too. Their bodies are parsed long after
// the enclosing function has been left, so the declarations that surrounded the
// local class have to be put back on the scope chain first.
int local_class() { struct Helper { int value() { return 20; } }; struct User { int use() { Helper h; return h.value() + 2; } }; User u; return u.use(); }
auto r5 = printf("local = %d\n", local_class());
// CHECK-NEXT: local = 22

// A class local to a template function works the same way; its bodies are
// parsed once per instantiation, with the template parameters back in scope.
template <typename T> T local_class_in_template() { struct Helper { T value() { return T(20); } }; struct User { T use() { Helper h; return h.value() + T(2); } }; User u; return u.use(); }
auto r6 = printf("local template = %d %.0f\n", local_class_in_template<int>(), local_class_in_template<double>());
// CHECK-NEXT: local template = 22 22

// Instantiating a local class no longer instantiates the members nobody calls,
// so a virtual one has to come back when the vtable marks it -- and the one
// that is only declared must still never be parsed.
template <typename T> T local_class_virtual() { struct Base { virtual T get() { return T(1); } virtual ~Base() {} }; struct Derived : Base { T get() { return T(7); } T unused() { this_member_does_not_exist(); return T(); } }; Derived d; Base *b = &d; return b->get(); }
auto r7 = printf("local virtual = %d\n", local_class_virtual<int>());
// CHECK-NEXT: local virtual = 7

// The body of a function that is never used is never parsed, so the errors it
// contains are never diagnosed.
int never_used() { this_function_does_not_exist(); return 0; }

auto r8 = printf("done\n");
// CHECK-NEXT: done

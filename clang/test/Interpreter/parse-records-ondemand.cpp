// With -fparse-records-ondemand a class body is parsed only when the type is
// first required to be complete.
// clang-format off
// RUN: cat %s | clang-repl -Xcc -fparse-records-ondemand | FileCheck %s
// RUN: cat %s | clang-repl -Xcc -fparse-records-ondemand -Xcc -fparse-functions-ondemand | FileCheck %s
// RUN: cat %s | clang-repl -Xcc -fparse-records-ondemand -Xcc -O2 | FileCheck %s

extern "C" int printf(const char *, ...);

// The body is parsed when an object of the type is created.
struct Point { int x; int y; int sum() const { return x + y; } };
auto r1 = printf("sum = %d size = %d\n", Point{3, 4}.sum(), (int)sizeof(Point));
// CHECK: sum = 7 size = 8

// The class is defined in an earlier partial translation unit than the one
// using it.
struct Counter { int n; int next() { return ++n; } };
auto r2 = printf("counter = %d %d\n", Counter{0}.next(), Counter{41}.next());
// CHECK-NEXT: counter = 1 42

// A member type is reached through a nested-name-specifier.
struct Traits { typedef int value_type; enum { Size = 3 }; static int tag() { return 9; } };
Traits::value_type r3 = printf("traits = %d %d\n", (int)Traits::Size, Traits::tag());
// CHECK-NEXT: traits = 3 9

// A deferred class used as a base class is completed for the base-clause, and
// the base-clause of the derived class is cached with its body.
struct Base { int b; };
struct Derived : Base { int d; };
auto r4 = printf("derived = %d\n", Derived{{5}, 6}.b + Derived{{5}, 6}.d);
// CHECK-NEXT: derived = 11

// The same through a vtable.
struct Shape { virtual int area() const { return 0; } virtual ~Shape() {} };
struct Square : Shape { int side; Square(int s) : side(s) {} int area() const override { return side * side; } };
int call_area(const Shape &s) { return s.area(); }
auto r4b = printf("area = %d\n", call_area(Square(5)));
// CHECK-NEXT: area = 25

// A deferred class used as a template argument.
template <typename T> struct Box { T value; };
struct Boxed { int v; };
auto r5 = printf("boxed = %d\n", Box<Boxed>{{12}}.value.v);
// CHECK-NEXT: boxed = 12

// The body of a class that is never needed complete is never parsed, so the
// errors it contains are never diagnosed.
struct NeverCompleted { this_type_does_not_exist member; };
NeverCompleted *never_completed;

auto r6 = printf("done\n");
// CHECK-NEXT: done

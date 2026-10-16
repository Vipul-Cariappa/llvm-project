// REQUIRES: host-supports-jit
// clang-repl can be called from the prompt in non-interactive mode as a
// calculator in shell scripts, for example. In that case if there is an error
// we should set the exit code as failure.
// RUN: not clang-repl "int x = 10;" "int y=7; err;" "int y = 10;"
// RUN: not clang-repl -Xcc -fparse-functions-ondemand "int x = 10;" "int y=7; err;" "int y = 10;"

// In interactive (REPL) mode, we can have errors but we should exit with
// success because errors in the input code are part of the interactive use.
// RUN: cat %s | clang-repl | FileCheck %s
// RUN: cat %s | clang-repl -Xcc -fparse-functions-ondemand | FileCheck %s

// However, interactive mode should fail when we specified -verify and there
// was a diagnostic mismatches. This will make the testsuite fail as intended.
// RUN: cat %s | not clang-repl -Xcc -Xclang -Xcc -verify | FileCheck %s
// RUN: cat %s | not clang-repl -Xcc -fparse-functions-ondemand -Xcc -Xclang -Xcc -verify | FileCheck %s

BOOM! // expected-error {{intended to fail the -verify test}}
extern "C" int printf(const char *, ...);
int i = 42;
auto r1 = printf("i = %d\n", i);
// CHECK: i = 42

1aap = 42; // expected-error {{invalid digit 'a' in decimal constant}}
1aap = 42; i = 5; // expected-error {{invalid digit 'a' in decimal constant}}

printf("i = %d\n", i);
// CHECK: i = 42

// Referencing something CodeGen emits lazily queues it up, and a failed input
// never reaches the point where the queue is drained. Dropping its module has
// to hand what it queued back, rather than emitting it (the module is going
// away) or forgetting it (a later input may reference it again).
template <class X> X lazy(X x) { return x * 2; }
int j = lazy(2); int k = undeclared_thing; // expected-error {{use of undeclared identifier 'undeclared_thing'}}
inline int inl(int x) { return x + 1; }
int l = inl(1); int m = undeclared_thing; // expected-error {{use of undeclared identifier 'undeclared_thing'}}
printf("inl = %d, lazy = %d\n", inl(3), lazy(4));
// CHECK: inl = 4, lazy = 8

// The same for a template that only fails once it is instantiated, where what
// the input queued up for emission is the specialization itself.
template <class X> int fails_when_instantiated(X x) { return x.no_such_member(); }
struct NoMember {};
int n = fails_when_instantiated(NoMember{}); // expected-error {{no member named 'no_such_member' in 'NoMember'}}
printf("i = %d\n", i);
// CHECK: i = 42

%quit

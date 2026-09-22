// -fparse-functions-ondemand defers function bodies and -fparse-records-ondemand
// defers class, struct and union bodies; both parse them on demand. Deferred
// bodies are never emitted if nothing uses them, which is only safe in
// incremental processing mode, so the flags are ignored (with a warning) unless
// -fincremental-extensions is enabled. They are also incompatible with
// serializing an AST, so they are ignored when generating a precompiled header
// or a module.

// RUN: rm -rf %t && split-file %s %t

// Without incremental processing the flags are ignored (with a warning).
// RUN: %clang_cc1 -fparse-functions-ondemand -fparse-records-ondemand -fsyntax-only -x c %t/hdr.h 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NOTINCREMENTAL

// When precompiling a header the flag is ignored (with a warning) and
// compilation succeeds even though 'f' would otherwise be emitted with an
// unparsed body.
// RUN: %clang_cc1 -fincremental-extensions -fparse-functions-ondemand -fparse-records-ondemand -emit-pch -x c-header %t/hdr.h -o %t/hdr.pch 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PCH

// When building a module the flag is ignored (with a warning).
// RUN: %clang_cc1 -fincremental-extensions -fparse-functions-ondemand -fparse-records-ondemand -emit-module -fmodules -fmodule-name=M \
// RUN:   -x c %t/module.modulemap -o %t/M.pcm 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MODULE

// A normal compilation honors the flag and emits no warning.
// RUN: %clang_cc1 -fincremental-extensions -fparse-functions-ondemand -fparse-records-ondemand -fsyntax-only -x c %t/hdr.h 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NORMAL --allow-empty

// NOTINCREMENTAL: warning: option '-fparse-functions-ondemand' is only supported with '-fincremental-extensions'; flag ignored
// NOTINCREMENTAL-NEXT: warning: option '-fparse-records-ondemand' is only supported with '-fincremental-extensions'; flag ignored
// PCH: warning: option '-fparse-functions-ondemand' is not supported when generating a precompiled header; flag ignored
// PCH-NEXT: warning: option '-fparse-records-ondemand' is not supported when generating a precompiled header; flag ignored
// MODULE: warning: option '-fparse-functions-ondemand' is not supported when generating a module; flag ignored
// MODULE-NEXT: warning: option '-fparse-records-ondemand' is not supported when generating a module; flag ignored
// NORMAL-NOT: flag ignored

//--- hdr.h
struct S { int x; };
__attribute__((used)) static int f(void) { return 42; }

//--- module.modulemap
module M { header "hdr.h" export * }

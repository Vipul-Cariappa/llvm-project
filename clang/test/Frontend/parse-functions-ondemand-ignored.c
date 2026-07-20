// -fparse-functions-ondemand defers function bodies and parses them on demand.
// That is incompatible with serializing an AST, so it is ignored (with a
// warning) when generating a precompiled header or a module, and honored
// otherwise.

// RUN: rm -rf %t && split-file %s %t

// When precompiling a header the flag is ignored (with a warning) and
// compilation succeeds even though 'f' would otherwise be emitted with an
// unparsed body.
// RUN: %clang_cc1 -fparse-functions-ondemand -emit-pch -x c-header %t/hdr.h -o %t/hdr.pch 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PCH

// When building a module the flag is ignored (with a warning).
// RUN: %clang_cc1 -fparse-functions-ondemand -emit-module -fmodules -fmodule-name=M \
// RUN:   -x c %t/module.modulemap -o %t/M.pcm 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MODULE

// A normal compilation honors the flag and emits no warning.
// RUN: %clang_cc1 -fparse-functions-ondemand -fsyntax-only -x c %t/hdr.h 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NORMAL --allow-empty

// PCH: warning: option '-fparse-functions-ondemand' is not supported when generating a precompiled header; flag ignored
// MODULE: warning: option '-fparse-functions-ondemand' is not supported when generating a module; flag ignored
// NORMAL-NOT: flag ignored

//--- hdr.h
__attribute__((used)) static int f(void) { return 42; }

//--- module.modulemap
module M { header "hdr.h" export * }

// RUN: %clang_cc1 -fincremental-extensions -fparse-records-ondemand -emit-llvm -triple %itanium_abi_triple -o - -std=c++11 -ffp-contract=off %s | FileCheck %s

// A deferred class body computes what the floating-point pragmas in effect
// where it was written say it computes, not what the ones in effect where the
// type was first needed say.

#pragma STDC FP_CONTRACT ON
struct Contracted {
  float f(float a, float b, float c) { return a * b + c; }
};
#pragma STDC FP_CONTRACT OFF

extern "C" float use(Contracted w) { return w.f(1, 2, 3); }

// -fparse-records-ondemand turns -fparse-functions-ondemand on, so a function
// nobody calls has no body to emit. Call it.
Contracted c;
float r = use(c);

// CHECK: call float @llvm.fmuladd.f32

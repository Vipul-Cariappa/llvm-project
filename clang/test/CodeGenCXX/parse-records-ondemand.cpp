// RUN: %clang_cc1 -fincremental-extensions -fparse-records-ondemand -emit-llvm -triple %itanium_abi_triple -o - -std=c++11 %s | FileCheck %s

// A class body deferred by -fparse-records-ondemand is often first needed
// complete inside an unevaluated operand. What the body declares belongs to the
// body, so it has to be parsed in a potentially-evaluated context of its own --
// otherwise the references in it are marked as non-odr-uses and CodeGen has
// nothing to emit for them.

struct NeededBySizeof {
  NeededBySizeof(int a) : m(a) {}
  int get() const { return m; }
  int m;
};

unsigned size = sizeof(NeededBySizeof);

extern "C" int use_after_sizeof() {
  NeededBySizeof n(3);
  return n.get();
}

struct NeededByDecltype {
  NeededByDecltype(int a) : m(a) {}
  int m;
};

using T = decltype(NeededByDecltype(0).m);

extern "C" int use_after_decltype() { return NeededByDecltype(4).m; }

// CHECK-DAG: define{{.*}} i32 @use_after_sizeof()
// CHECK-DAG: define{{.*}} i32 @use_after_decltype()

// The constructor initializes from its parameter, and the accessor loads the
// member, rather than both being empty.
// CHECK-DAG: define{{.*}} @_ZN14NeededBySizeofC2Ei(ptr{{.*}} %this, i32 noundef %a)
// CHECK-DAG: define{{.*}} @_ZN16NeededByDecltypeC2Ei(ptr{{.*}} %this, i32 noundef %a)
// CHECK-DAG: define{{.*}} @_ZNK14NeededBySizeof3getEv

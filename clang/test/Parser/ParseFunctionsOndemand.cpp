// RUN: %clang_cc1 -fparse-functions-ondemand -fsyntax-only -verify -Wall -Wextra -Werror -ferror-limit 0 -std=c++11 %s
// RUN: not %clang_cc1 -fparse-functions-ondemand -fdelayed-template-parsing -fsyntax-only -std=c++11 %s 2>&1 | FileCheck %s --check-prefix=MUTEX

// MUTEX: error: invalid argument '-fparse-functions-ondemand' not allowed with '-fdelayed-template-parsing'

namespace InternalFunctionBodies {
static void static_diagnoses_on_use() {
  undeclared_static(); // expected-error {{use of undeclared identifier 'undeclared_static'}}
}

static void prior_static();
void prior_static() {
  undeclared_prior_static(); // expected-error {{use of undeclared identifier 'undeclared_prior_static'}}
}

namespace {
void anon_namespace_function() {
  undeclared_anon_namespace(); // expected-error {{use of undeclared identifier 'undeclared_anon_namespace'}}
}
} // namespace

void external_function_diagnoses_eagerly() {
  undeclared_external(); // expected-error {{use of undeclared identifier 'undeclared_external'}}
}

static void unreferenced_static_body_is_delayed() {
  undeclared_but_unreferenced(); // this should not give an error
}

void trigger_internal_function_bodies() {
  static_diagnoses_on_use();
  prior_static();
  anon_namespace_function();
}
} // namespace InternalFunctionBodies

namespace SourceOrderOverloads {
struct One {};
struct Two {}; // expected-note 2 {{candidate constructor}}

static One foo_hidden_later(int);
static void call_before_later_overload() {
  Two value = foo_hidden_later(0.0); // expected-error {{no viable conversion from 'One' to 'Two'}}
}
static Two foo_hidden_later(double);

static Two foo_forward_declared(double);
static void call_forward_declared_overload() {
  Two _ = foo_forward_declared(0.0);
}
static Two foo_forward_declared(double);

static One foo_both_before(int);
static Two foo_both_before(double);
static void call_both_overloads_before() {
  Two _ = foo_both_before(0.0);
}

void trigger_source_order_overloads() {
  call_before_later_overload();
  call_forward_declared_overload();
  call_both_overloads_before();
}
} // namespace SourceOrderOverloads

namespace NamespaceScopeSourceOrder {
static int later_variable_hidden() {
  return later_variable; // expected-error {{use of undeclared identifier 'later_variable'}}
}
static int later_variable;

static int later_function_hidden() {
  return later_function(); // expected-error {{use of undeclared identifier 'later_function'}}
}
static int later_function();

static int builtin_lookup_still_works() {
  return __builtin_abs(-1);
}

void trigger_namespace_scope_source_order() {
  later_variable_hidden();
  later_function_hidden();
  builtin_lookup_still_works();
}
} // namespace NamespaceScopeSourceOrder

namespace ClassMethodLookup {
struct NormalClass {
  void sees_later_method() {
    later_method();
  }
  void later_method();

  int sees_later_static_member() {
    return later_static_member;
  }
  static int later_static_member;
};

namespace {
struct InternalClass {
  void unreferenced_body_is_delayed() {
    undeclared_in_unreferenced_method(); // this should not give an error
  }

  void sees_later_member() {
    later_member();
  }
  void later_member() {}

  void diagnoses_on_use() {
    undeclared_in_referenced_method(); // expected-error {{use of undeclared identifier 'undeclared_in_referenced_method'}}
  }

  void later_global_hidden() {
    later_global_after_class; // expected-error {{use of undeclared identifier 'later_global_after_class'}}
  }

  static int static_member_later_global_hidden() {
    return later_global_after_class; // expected-error {{use of undeclared identifier 'later_global_after_class'}}
  }

  static int static_member_sees_later_static_member() {
    return later_static_member;
  }
  static int later_static_member;
};
} // namespace

int InternalClass::later_static_member = 10;
int later_global_after_class = 10;

void trigger_class_method_lookup() {
  InternalClass object;
  object.sees_later_member();
  object.diagnoses_on_use();
  object.later_global_hidden();
  InternalClass::static_member_later_global_hidden();
  InternalClass::static_member_sees_later_static_member();
}
} // namespace ClassMethodLookup

namespace OutOfClassMethods {
namespace {
struct S {
  int sees_prior_global();
  int hides_later_global();
};

int prior_global; // expected-note {{'prior_global' declared here}}

int S::sees_prior_global() {
  return prior_global;
}

int S::hides_later_global() {
  return later_global; // expected-error {{use of undeclared identifier 'later_global'}}
}

int later_global; // expected-note 2 {{'OutOfClassMethods::later_global' declared here}}
} // namespace

void trigger_out_of_class_methods() {
  S s;
  s.sees_prior_global();
  s.hides_later_global();
}
} // namespace OutOfClassMethods

namespace FriendFunctionsAndClasses {
int prior_global;

namespace {
struct FriendFunctionPrior {
  friend int friend_function_prior(FriendFunctionPrior) {
    return prior_global;
  }
};

struct FriendFunctionLater {
  friend int friend_function_later(FriendFunctionLater) {
    return later_global; // expected-error {{use of undeclared identifier 'later_global'}}
  }
};

struct Host {
  friend struct FriendClass;
};

struct FriendClass {
  static int sees_prior_global() {
    return prior_global;
  }
  static int hides_later_global() {
    return later_global; // expected-error {{use of undeclared identifier 'later_global'}}
  }
};
} // namespace

int later_global;

void trigger_friend_functions_and_classes() {
  friend_function_prior(FriendFunctionPrior{});
  friend_function_later(FriendFunctionLater{});
  FriendClass::sees_prior_global();
  FriendClass::hides_later_global();
}
} // namespace FriendFunctionsAndClasses

namespace ADLLookup {
struct One {};
struct Two {};

namespace Hidden {
struct A {};
}

static void adl_later_function_hidden() {
  Hidden::A a;
  Two value = adl_target(a); // expected-error {{use of undeclared identifier 'adl_target'}}
}

namespace Hidden {
One adl_target(A);
}

namespace Visible {
struct A {};
One adl_target(A);
}

static void adl_prior_function_visible() {
  Visible::A a;
  One _ = adl_target(a);
}

void trigger_adl_lookup() {
  adl_later_function_hidden();
  adl_prior_function_visible();
}
} // namespace ADLLookup

namespace QualifiedLookup {
struct One {};
struct Two {};

namespace Hidden {}

static void qualified_later_function_hidden() {
  Hidden::h(); // expected-error {{no member named 'h' in namespace 'QualifiedLookup::Hidden'}}
}

namespace Hidden {
One h();
}

namespace Visible {
One h();
}

static void qualified_prior_function_visible() {
  One _ = Visible::h();
}

void trigger_qualified_lookup() {
  qualified_later_function_hidden();
  qualified_prior_function_visible();
}
} // namespace QualifiedLookup

namespace DeclarationParts {
static int default_argument(int value = default_argument_later_global) { // expected-error {{use of undeclared identifier 'default_argument_later_global'}}
  return value;
}
int default_argument_later_global;

static void noexcept_expr() noexcept(noexcept(noexcept_later_global)) {} // expected-error {{use of undeclared identifier 'noexcept_later_global'}}
int noexcept_later_global;

static auto trailing_return() -> decltype(trailing_return_later_global) { // expected-error {{use of undeclared identifier 'trailing_return_later_global'}}
  return 0;
}
int trailing_return_later_global;

void deleted_function() = delete;

struct DefaultedConstructor {
  DefaultedConstructor() = default;
};
} // namespace DeclarationParts

namespace LocalClassCases {
void local_class_method_diagnoses_on_use() {
  struct S {
    void f() {
      undeclared_in_local_class(); // expected-error {{use of undeclared identifier 'undeclared_in_local_class'}}
    }
  };

  S s;
  s.f();
}

void local_class_method_sees_local_typedef() {
  typedef int T;
  struct S {
    T f() { return 0; }
  };

  S s;
  s.f();
}
} // namespace LocalClassCases

namespace TemplateCases {
template <class T>
void template_body_diagnoses_on_instantiation() {
  undeclared_in_template(); // expected-error {{use of undeclared identifier 'undeclared_in_template'}}
}

void trigger_template_cases() {
  template_body_diagnoses_on_instantiation<int>();
}
} // namespace TemplateCases

// RUN: %clang_cc1 -fincremental-extensions -fparse-records-ondemand -fsyntax-only -verify -Wall -Wextra -Werror -ferror-limit 0 -std=c++11 %s
// RUN: %clang_cc1 -fincremental-extensions -fparse-records-ondemand -fsyntax-only -verify -Wall -Wextra -Werror -ferror-limit 0 -std=c++17 %s

// With -fparse-records-ondemand the member-specification of a namespace-scope
// class is consumed but not parsed. The class stays a forward declaration until
// something first requires the type to be complete, and only then are the
// members parsed.

// A class that is never required complete keeps its body unparsed, so the
// errors in it are never diagnosed.
namespace NeverCompleted {
struct Unused {
  undeclared_type_in_unused_body member;
  void f() { undeclared_in_unused_body(); }
};

// Naming the type without needing it complete is not enough either.
struct OnlyPointedTo {
  another_undeclared_type member;
};
OnlyPointedTo *pointer;
OnlyPointedTo &reference(OnlyPointedTo *p) { return *p; }
} // namespace NeverCompleted

// A class body is parsed when the type is first required to be complete, and
// the errors in it are diagnosed then.
namespace DiagnosesOnUse {
struct Bad {
  undeclared_type_in_used_body member; // expected-error {{unknown type name 'undeclared_type_in_used_body'}}
};

Bad object;
} // namespace DiagnosesOnUse

// Every way of needing the type complete parses the body.
namespace CompletionTriggers {
struct ByObject { int x; };
ByObject by_object;

struct BySizeof { int x; };
unsigned by_sizeof = sizeof(BySizeof);

struct ByMemberAccess { int x; };
int by_member_access(ByMemberAccess *p) { return p->x; }

struct ByBase { int x; };
struct Derives : ByBase {};
int by_base(Derives d) { return d.x; }

struct ByQualifiedLookup { typedef int Alias; static const int Value = 1; };
ByQualifiedLookup::Alias by_qualified_lookup = ByQualifiedLookup::Value;

struct ByNestedClass { struct Nested { int n; }; };
ByNestedClass::Nested by_nested_class;

struct ByEnumerator { enum E { Value }; };
int by_enumerator = ByEnumerator::Value;

struct ByMemberFunction { int x; int get() const { return x; } };
int by_member_function(const ByMemberFunction &b) { return b.get(); }

struct ByTemplateArgument { int x; };
template <typename T> struct Box { T value; };
int by_template_argument(Box<ByTemplateArgument> b) { return b.value.x; }
} // namespace CompletionTriggers

// A class that names itself in its own body is parsed exactly once.
namespace SelfReference {
struct Node {
  Node *next;
  int value;
  int sum() const { return next ? value + next->sum() : value; }
};
int use(Node n) { return n.sum(); }
} // namespace SelfReference

// Two classes that refer to each other through pointers.
namespace MutualReference {
struct B;
struct A { B *b; int a; };
struct B { A *a; int b; };
int use(A x, B y) { return x.a + y.b; }
} // namespace MutualReference

// A declaration of the class before its definition does not make the body
// eager, and a definition after one is still the definition.
namespace ForwardDeclared {
struct S;
struct S { int x; };
int use(S s) { return s.x; }
} // namespace ForwardDeclared

// A second definition is still a redefinition, even though the first body had
// not been parsed when the second one was seen.
namespace Redefinition {
struct S { int a; }; // expected-note {{previous definition is here}}
struct S { int b; }; // expected-error {{redefinition of 'S'}}
} // namespace Redefinition

// A class that never gets a definition is still diagnosed as incomplete.
namespace StillIncomplete {
struct Undefined; // expected-note {{forward declaration of 'StillIncomplete::Undefined'}}
void use() {
  Undefined u; // expected-error {{variable has incomplete type 'Undefined'}}
}
} // namespace StillIncomplete

// A deferred body is parsed long after the declarations that follow it have
// been seen, but the cached tokens carry their original locations, so source
// order still decides what the names in it can see.
namespace SourceOrder {
struct Earlier { int x; };

struct SeesPrior {
  Earlier *prior; // fine, 'Earlier' is declared above
  int x;
};
SeesPrior sees_prior;

struct DoesNotSeeLater {
  Later *later; // expected-error {{unknown type name 'Later'}}
  int x;
};
DoesNotSeeLater does_not_see_later;

struct Later { int x; };

// A member of the class is found whatever the order, because the body of a
// member function is a complete-class context.
struct MemberOrder {
  int first() { return second(); }
  int second() { return 1; }
};
int member_order(MemberOrder m) { return m.first(); }

// The same holds one class further in: it is the nested class whose members
// are visible out of order, not the one whose body was deferred.
struct NestedMemberOrder {
  struct Nested {
    Nested(int a) : m(a) {}
    int get() { return later(); }
    int m;
    int later() { return m; }
  };
  Nested n;
};
int nested_member_order(NestedMemberOrder o) { return o.n.get(); }
// A declaration in a deferred body is a declaration of the entity further down
// the file, not of a new one. Source order does not come into it: hiding the
// later declaration here would befriend a second, unrelated 'Iter'.
namespace RedeclarationInDeferredBody {
struct Ref {
public:
  int value() const { return m; }

private:
  friend struct Iter;
  Ref(int a) : m(a) {}
  int m;
};

struct Iter {
  Ref get() { return Ref(1); }
};

int use(Iter i) { return i.get().value(); }
} // namespace RedeclarationInDeferredBody
} // namespace SourceOrder

// The base-clause is cached with the body, so a class with bases is deferred
// too. Finding where the body starts means looking past the base-clause
// without parsing it, which the '{' of a braced-init-list inside a template
// argument could otherwise be mistaken for.
namespace WithBaseClause {
struct Base { int b; };
struct Other { int o; };
template <typename T> struct Wrap { T t; };
template <int N> struct Number { int n; };

struct Simple : Base { int d; };
int simple(Simple s) { return s.b + s.d; }

struct Several : public Base, protected Other { int d; };
int several(Several s) { return s.d; }

struct Virtual : virtual Base { int d; };
int virtual_base(Virtual v) { return v.b; }

struct NestedAngles : Wrap<Wrap<int>> { int d; };
int nested_angles(NestedAngles n) { return n.t.t; }

struct BraceInTemplateArgument : Wrap<decltype(Base{})> { int d; };
int brace_in_template_argument(BraceInTemplateArgument b) { return b.t.b; }

struct LessThanInTemplateArgument : Number<(1 < 2)> { int d; };
int less_than_in_template_argument(LessThanInTemplateArgument l) { return l.n; }

struct Final final : Base { int d; };
int final_with_base(Final f) { return f.b; }

struct FinalWithoutBase final { int d; };
int final_without_base(FinalWithoutBase f) { return f.d; }

// The base-clause of a class that is never completed is not parsed either.
struct NeverCompleted : Base {
  undeclared_type_under_a_base member;
};
NeverCompleted *never_completed;
} // namespace WithBaseClause

// A primary class template is deferred as well; its body is parsed when the
// template is first instantiated, with its parameter scopes back in place.
namespace ClassTemplates {
template <typename T> struct Box { T v; T get() const { return v; } };
int instantiated(Box<int> b) { return b.get(); }

// Never instantiated, so the body is never parsed and the errors in it are
// never diagnosed.
template <typename T> struct NeverInstantiated {
  undeclared_type_in_template_body member;
  T t;
};
NeverInstantiated<int> *never_instantiated;

// One body, two instantiations.
template <typename T> struct Twice { T v; };
int twice(Twice<int> a, Twice<char> b) { return a.v + b.v; }

// A default template argument and a non-type parameter.
template <typename T, int N = 4> struct Array {
  T data[N];
  int size() const { return N; }
};
int array_size(Array<int> a) { return a.size(); }

// The base-clause names the template parameter, so it has to be parsed with the
// parameter scopes restored.
template <typename T> struct TemplateBase { T b; };
template <typename T> struct TemplateDerived : TemplateBase<T> { T d; };
int derived(TemplateDerived<int> d) { return d.d; }

// A member template of a deferred template is parsed with it.
template <typename T> struct WithMemberTemplate {
  template <typename U> struct Inner { U u; };
  Inner<int> i;
};
int member_template(WithMemberTemplate<char> w) { return w.i.u; }

// An explicit specialization does not need the primary's body, and the primary
// is still deferred for the other arguments.
template <typename T> struct Specialized { int which() { return 1; } };
template <> struct Specialized<int> { int which() { return 2; } };
int specialized() { return Specialized<char>().which() + Specialized<int>().which(); }

// A partial specialization, likewise.
template <typename T> struct Partial { int which() { return 1; } };
template <typename T> struct Partial<T *> { int which() { return 2; } };
int partial() { return Partial<char>().which() + Partial<char *>().which(); }

// An explicit instantiation parses the body too.
template <typename T> struct Explicit { T t; };
template struct Explicit<int>;

// An out-of-line member definition names the pattern through a dependent
// nested-name-specifier, which does not go through RequireCompleteDeclContext.
template <typename T> struct OutOfLine {
  T f();
  static int s;
  typedef T Alias;
};
template <typename T> T OutOfLine<T>::f() { return T(); }
template <typename T> int OutOfLine<T>::s = 0;
int out_of_line(OutOfLine<int> o) { return o.f() + OutOfLine<int>::s; }

// A second definition of a template is still a redefinition.
template <typename T> struct Redefined { int a; }; // expected-note {{previous definition is here}}
template <typename T> struct Redefined { int b; }; // expected-error {{redefinition of 'Redefined'}}

// The body names a specialization of its own template, so the body has to be
// parsed before the instantiation that wanted it starts -- otherwise the
// recursion guard turns the nested use into an incomplete type.
template <int N> struct SelfInstantiating {
  void f() { SelfInstantiating<1> s; (void)s; }
};
void self_instantiating(SelfInstantiating<1> s) { s.f(); }

#if __cplusplus >= 201703L
// Class template argument deduction builds its implicit guides from the
// constructors, so the body has to be parsed before the guides are formed.
template <typename T> struct Deduced {
  Deduced(T);
  T value;
};
void deduction() {
  Deduced d(1);
  (void)d.value;
}
#endif
} // namespace ClassTemplates

// A member of a class is subject to the complete-class rule, not to source
// order: a body, a default argument or an initializer in a class sees every
// member of it, and of the classes nested in it.
namespace CompleteClassContext {
struct Outer {
  int line() const { return impl->value; }
  int direct() const { return later; }
  struct Impl { int value; };
  const Impl *impl;
  int later;
};
int complete_class_context(Outer o) { return o.line() + o.direct(); }
} // namespace CompleteClassContext

// A class is often first needed complete inside a function, and inside a
// lambda in particular. Its members are not being written in that lambda, so
// the scopes of whatever the parser was in the middle of are put aside for the
// late parse.
namespace InsideALambda {
struct NeededInLambda {
  int m = 1;
  int get() { return this->m; }
};

auto lambda = []() {
  NeededInLambda n;
  return n.get();
};

struct NeededInNestedLambda {
  int m = 2;
  int get() { return this->m; }
};

auto nested = []() {
  return [](int x) {
    NeededInNestedLambda n;
    return n.get() + x;
  }(1);
};
} // namespace InsideALambda

// The template parameter depth of a deferred body is the depth where it was
// written, not the depth of whatever first asked for the type.
namespace TemplateDepth {
typedef unsigned long size_t;

struct Selector {
  template <typename...> struct List { };
  template <typename T, typename... U> struct List<T, U...> : List<U...> {
    static const size_t size = sizeof(T);
  };
  typedef List<unsigned char, unsigned short> UInts;

  template <size_t Sz, typename T, bool = (Sz <= T::size)> struct Pick;
  template <size_t Sz, typename U, typename... Us>
  struct Pick<Sz, List<U, Us...>, true> { typedef U type; };
  template <size_t Sz, typename U, typename... Us>
  struct Pick<Sz, List<U, Us...>, false> : Pick<Sz, List<Us...> > { };
};

// Reached from inside a template, so the enclosing depth is not zero.
template <typename T> struct Uses : Selector {
  typedef typename Pick<sizeof(T), UInts>::type type;
};
Uses<char>::type template_depth = 0;
} // namespace TemplateDepth

// Forms that are not deferred are parsed as they always were.
namespace NotDeferred {
// An unnamed class has no name to look up, so it cannot be deferred.
typedef struct { int t; } Unnamed;
Unnamed unnamed;

// A member class is parsed with its enclosing class.
struct Enclosing {
  struct Member { int m; };
  Member member;
};
Enclosing enclosing;

// A local class is parsed with its enclosing function.
void local() {
  struct Local { int l; };
  Local l;
  (void)l;
}
} // namespace NotDeferred

// A declarator after the member-specification needs the type complete, so the
// body is parsed right away.
namespace DefinitionWithDeclarator {
struct WithObject {
  undeclared_type_after_declarator member; // expected-error {{unknown type name 'undeclared_type_after_declarator'}}
} object;
} // namespace DefinitionWithDeclarator

// Attributes written after the '}' are cached with the body and applied when it
// is parsed.
namespace TrailingAttributes {
struct Packed {
  char c;
  int i;
} __attribute__((packed));
static_assert(sizeof(Packed) == 5, "trailing attribute was lost");
} // namespace TrailingAttributes

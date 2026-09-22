// RUN: %clang_cc1 -fincremental-extensions -fparse-records-ondemand -fsyntax-only -verify -Wall -Wextra -Werror -ferror-limit 0 -std=c++11 %s

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

// Forms that are not deferred are parsed as they always were.
namespace NotDeferred {
// An unnamed class has no name to look up, so it cannot be deferred.
typedef struct { int t; } Unnamed;
Unnamed unnamed;

// A class template needs its parameter scopes back, which the deferred parse
// does not restore.
template <typename T> struct Template { T value; };
Template<int> instantiated;

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

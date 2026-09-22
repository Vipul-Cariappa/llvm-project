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

// Forms that are not deferred are parsed as they always were.
namespace NotDeferred {
// An unnamed class has no name to look up, so it cannot be deferred.
typedef struct { int t; } Unnamed;
Unnamed unnamed;

// A base-clause has to be parsed in the context it was written in.
struct WithBaseSource { int x; };
struct WithBase : WithBaseSource { int y; };
WithBase with_base;

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

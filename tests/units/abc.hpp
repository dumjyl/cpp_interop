namespace blah {
class Foo {
   int a;
   int b;

   void dont_bind() {}

   public:
   Foo* recursive_field;

   Foo(int a, int b) : a(a) { this->b = b; }

   ~Foo() { auto y = a + b; }

   void recusive_meth(const Foo* x) {}

   int calc(int x) { return x + a + b; }

   static Foo init(int a = 12) { return Foo(a, a * 2); }
};

using VP = void*;

auto sum(float a, float b) -> float { return a + b; }

blah::Foo x(1, 2);
} // namespace blah

enum class SomeEnum { a, b, c };

typedef struct TypedefTest {
   typedef struct TypedefNamedNamed {
   } TypedefNamedNamed;

   typedef struct {
   } TypedefAnonNamed;

   struct TypedefNamedAnon {};
} TypedefTest;

struct XYZ {
   union {
   } xyz_field;
};

union {
} anon_union_var;

int __your_identifiers_are__the___worst_;
int _abc__a;
int abc_a_;
int abc_a___;
int __;
int _;

struct SepTypedef {};

typedef struct SepTypedef SepTypedef;

using FnPtr = void (*)(int a);
// Idk if anyone uses these. If they are, some valid operations on them won't work in nim.
// Probably doesn't matter since they are so niche.
using FnRef = void (&)(int a);
using FnRValueRef = void(&&)(int a);


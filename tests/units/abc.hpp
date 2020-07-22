namespace blah {
class Foo {
   int a;
   int b;
   void dont_bind(){};

   public:
   Foo(int a, int b) : a(a) { this->b = b; }

   ~Foo() { auto y = a + b; }

   int calc(int x) { return x + a + b; }
};

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

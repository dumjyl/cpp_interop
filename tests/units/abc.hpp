namespace blah {
class Foo {
   int a;
   int b;
   void dont_bind(){};

   public:
   Foo(int a, int b) : a(a) { this->b = b; }

   int calc(int x) { return x + a + b; }
};
blah::Foo x(1, 2);
} // namespace blah

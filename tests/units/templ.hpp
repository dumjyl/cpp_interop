#include <cstdint>

template <typename T> T foo(T a, T b) { return a + b; }

template <std::size_t size, typename T> class Vec {
   public:
   T data[size];

   Vec(T val) {
      for (std::size_t i = 0; i < size; i += 1) {
         data[i] = val;
      }
   }

   template <typename U> Vec(T val, U nop_val) {
      for (std::size_t i = 0; i < size; i += 1) {
         data[i] = val;
      }
   }

   template <typename U> void nop(Vec<size, U> val) {}

   T& operator[](std::size_t i) { return data[i]; }

   template <typename U> static int double_size(U val) { return size * 2; }
};

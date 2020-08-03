#pragma once

#include <memory>
#include <type_traits>

namespace ensnare::runtime {
template <typename T> using UnsizedArray = T[];
template <typename T> using Constant = std::add_const_t<T>;

/// Do not use. An unsafe POD buffer managed by nim destructors.
template <typename T> class LaunderClassBuf {
   private:
   bool live; // We need this flag to not destroy an unitialized object or
              // an already destroyed object.// Both of which are
              // disallowed by the c++ memory model. I think?
              //
              // FIXME(optimize), we could use unused bits for cheap
              // wrapper classes. expoese it with a nice api.
   std::aligned_storage_t<sizeof(T)> buf; // Storage for one class.

   T& read_buf() {
      // reinterpret the address of the buffer as a pointer
      return *std::launder(reinterpret_cast<T*>(&this->buf));
   }

   public:
   template <typename... Args> LaunderClassBuf(Args... args) {
      // forward varariadic args to placement new construction.
      new (&this->buf) T(std::forward<Args>(args)...);
      this->live = true; // we have a live object that must be destroyed.
   }

   void unsafe_destroy() {
      if (this->live) { // if have an initialized object we shall destroy it
                        // and disarm the buffer.
         this->read_buf().~T();
         this->live = false;
      }
   }

   void unsafe_move(LaunderClassBuf<T>& other) {
      this->read_buf() = std::move(other.read_buf());
      other.live = false;
   }

   // A binding generator would only expose this if the type supported it.
   LaunderClassBuf<T> unsafe_copy() { return this->read_buf(); }

   T& unsafe_deref() { return this->read_buf(); }
};
} // namespace ensnare::runtime

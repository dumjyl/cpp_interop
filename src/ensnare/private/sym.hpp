#pragma once

#include "ensnare/private/utils.hpp"

namespace ensnare {
/// Used within a Sym.
class SymObj {
   private:
   Vec<Str> detail;
   bool _no_stropping; ///< This symbol does not require stropping even if it is a keyword.
   public:
   SymObj(Str name, bool no_stropping);
   void update(Str name);
   const Str& latest() const;
   bool no_stropping() const;
};

/// This is reference counted string class with a history.
using Sym = Node<SymObj>;

Sym new_Sym(Str name, bool no_stropping = false);
} // namespace ensnare

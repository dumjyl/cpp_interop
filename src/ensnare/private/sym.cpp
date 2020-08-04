#include "ensnare/private/sym.hpp"

using namespace ensnare;

ensnare::SymObj::SymObj(Str name, bool no_stropping)
   : detail({name}), _no_stropping(no_stropping) {}

void ensnare::SymObj::update(Str name) { detail.push_back(name); }

const Str& ensnare::SymObj::latest() const { return detail.back(); }

bool ensnare::SymObj::no_stropping() const { return _no_stropping; }

Sym ensnare::new_Sym(Str name, bool no_stropping) { return node<SymObj>(name, no_stropping); }

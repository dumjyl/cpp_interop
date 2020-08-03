#pragma once

#include "ensnare/private/utils.hpp"

#include <random>

namespace ensnare {
using Seed = std::random_device::result_type;

Seed non_deterministic_seed();

/// Generates symbols for mangling purposes.
class SymGenerator {
   std::mt19937 rng;
   Vec<char> chars;
   std::uniform_int_distribution<Size> sampler;
   char sample_char();
   public:
   SymGenerator(Seed seed = non_deterministic_seed());
   Str operator()(Size size);
};
} // namespace ensnare

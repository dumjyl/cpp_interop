#include "ensnare/private/sym_generator.hpp"

using namespace ensnare;

Seed ensnare::non_deterministic_seed() {
   std::random_device entropy;
   return entropy();
}

namespace ensnare {
Vec<char> init_chars() {
   Vec<char> result;
   for (auto i = static_cast<int>('a'); i < static_cast<int>('z'); i += 1) {
      result.push_back(static_cast<char>(i));
   }
   for (auto i = static_cast<int>('0'); i < static_cast<int>('9'); i += 1) {
      result.push_back(static_cast<char>(i));
   }
   return result;
}
} // namespace ensnare

ensnare::SymGenerator::SymGenerator(Seed seed)
   : rng(seed), chars(init_chars()), sampler(0, chars.size() - 1) {}

char ensnare::SymGenerator::sample_char() { return chars[sampler(rng)]; }

Str ensnare::SymGenerator::operator()(Size size) {
   Str result;
   for (auto i = 0; i < size; i += 1) {
      result.push_back(sample_char());
   }
   return result;
}

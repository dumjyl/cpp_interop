#include "ensnare/private/str_utils.hpp"

#include "ensnare/private/bit_utils.hpp"

using namespace sugar;

Vec<Str> ensnare::split_newlines(const Str& str) {
   Vec<Str> result;
   auto last = 0;
   for (auto i = 0; i < str.size(); i += 1) {
      if (str[i] == '\n') {
         Str str_result;
         for (auto j = last; j < i; j++) {
            str_result.push_back(str[j]);
         }
         result.push_back(str_result);
         last = i + 1;
      }
   }
   Str str_result;
   for (auto j = last; j < str.size(); j++) {
      str_result.push_back(str[j]);
   }
   return result;
}

bool ensnare::ends_with(const Str& self, const Str& suffix) {
   if (suffix.size() > self.size()) {
      return false;
   } else {
      for (auto i = 0; i < suffix.size(); i += 1) {
         if (self[self.size() - suffix.size() + i] != suffix[i]) {
            return false;
         }
      }
      return true;
   }
}

bool is_find_str(const char* data, Size data_size, const Str& find) {
   if (data_size >= find.size()) {
      for (auto i = 0; i < find.size(); i += 1) {
         if (data[i] != find[i]) {
            return false;
         }
      }
      return true;
   } else {
      return false;
   }
}

Str ensnare::replace(const Str& str, const Str& find, const Str& replace) {
   Str result;
   std::size_t i = 0;
   while (i < str.size()) {
      if (is_find_str(&str[i], str.size() - i - 1, find)) {
         result += replace;
         i += find.size();
      } else {
         result += str[i];
         i += 1;
      }
   }
   return result;
}

void incl(ensnare::CharSet& chars, char low, char high) {
   for (auto i = static_cast<std::size_t>(low); i <= high; i += 1) {
      chars.incl(static_cast<char>(i));
   }
}

ensnare::CharSet ident_chars() {
   ensnare::CharSet result;
   incl(result, 'A', 'Z');
   incl(result, 'a', 'z');
   incl(result, '0', '9');
   result.incl('_');
   return result;
}

bool ensnare::is_ident_chars(const Str& str) {
   for (auto c : str) {
      if (!ident_chars()[c]) {
         return false;
      }
   }
   return true;
}

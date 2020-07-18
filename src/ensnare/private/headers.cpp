#include "ensnare/private/headers.hpp"

#include "ensnare/private/os_utils.hpp"
#include "ensnare/private/str_utils.hpp"

#include <algorithm>

//
#include "ensnare/private/syn.hpp"

fn ensnare::Header::system(const Str name) -> Header {
   auto result = Header(name);
   result.is_system = true;
   return result;
}

ensnare::Header::Header(const Str name) : is_system(false), name(name) {}

namespace ensnare {
// Get a suitable location to store temp file.
fn temp(Str name) -> Str { return os::temp_file("ensnare_system_includes_" + name); }

// Get some verbose compiler logs to parse.
fn get_raw_include_paths() -> Str {
   os::write_file(temp("test"), ""); // so the compiler does not error.
   Str cmd = "clang++ -xc++ -c -v " + temp("test") + " 2>&1";
   return os::successful_process_output(cmd);
}
} // namespace ensnare

fn ensnare::include_paths() -> Vec<Str> {
   Vec<Str> result;
   auto lines = split_newlines(get_raw_include_paths());
   if (lines.size() == 0) {
      return result;
   }
   auto start = std::find(lines.begin(), lines.end(), "#include <...> search starts here:");
   auto stop = std::find(lines.begin(), lines.end(), "End of search list.");
   if (start == lines.end() || stop == lines.end()) {
      fatal("failed to parse include paths");
   }
   for (auto it = start + 1; it != stop; it++) {
      if (it->size() > 0) {
         result.push_back("-isystem" + ((*it)[0] == ' ' ? it->substr(1) : *it));
      }
   }
   return result;
}

#include "ensnare/private/undef_syn.hpp"

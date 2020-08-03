#include "ensnare/private/headers.hpp"

#include "ensnare/private/str_utils.hpp"
#include "sugar/os_utils.hpp"

#include <algorithm>

using namespace sugar;
using namespace ensnare;

ensnare::Header::Header(const Str name) : is_system(false), name(name) {}

Header ensnare::Header::system(const Str name) {
   auto result = Header(name);
   result.is_system = true;
   return result;
}

bool is_system_header(const Str& header) {
   return (header.size() > 2 && header[0] == '<' and header[header.size() - 1] == '>');
}

ensnare::Header::operator Str() const { return name; }

Opt<Header> ensnare::Header::parse(const Str& str) {
   if (is_system_header(str)) {
      return Header::system(str.substr(1, str.size() - 2));
   } else {
      auto str_ext = Path(str).extension();
      for (const auto& ext : {".hpp", ".cpp", ".h", ".c"}) {
         if (str_ext == ext) {
            return str;
         }
      }
      return {};
   }
}

namespace ensnare {
// Get a suitable location to store temp file.
Str temp(Str name) { return fs::temp_directory_path() / ("ensnare_system_includes_" + name); }

// Get some verbose compiler logs to parse.
Str get_raw_include_paths(const Str& cmd_start) {
   // so the compiler does not error.
   require(write_file(temp("test"), ""), "failed to write system include test file");
   // FIXME: this is a pretty terrible check.
   Str cmd = cmd_start + " -c -v " + temp("test") + " -o " + temp("test.o") + " 2>&1";
   return successful_process_output(cmd);
}

Vec<Str> search_paths(const Str& cmd_start) {
   Vec<Str> result;
   auto lines = split_newlines(get_raw_include_paths(cmd_start));
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
         result.push_back(((*it)[0] == ' ' ? it->substr(1) : *it));
      }
   }
   return result;
}
} // namespace ensnare

Vec<Str> ensnare::Header::search_paths() {
   // FIXME: expose the compiler/lang as an option.
   return ensnare::search_paths("clang++ -xc++");
}

Str ensnare::Header::render() const {
   return "#include " + (is_system ? "<" + name + ">" : "\"" + name + "\"") + "\n";
}

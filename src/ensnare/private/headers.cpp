#include "ensnare/private/headers.hpp"

#include "ensnare/private/os_utils.hpp"
#include "ensnare/private/str_utils.hpp"

#include <algorithm>

//
#include "ensnare/private/syn.hpp"

ensnare::Header::Header(const Str name) : is_system(false), name(name) {}

fn ensnare::Header::system(const Str name) -> Header {
   auto result = Header(name);
   result.is_system = true;
   return result;
}

fn is_system_header(const ensnare::Str& header) -> bool {
   return (header.size() > 2 && header[0] == '<' and header[header.size() - 1] == '>');
}

ensnare::Header::operator Str() const { return name; }

fn ensnare::Header::parse(const Str& str) -> Opt<Header> {
   if (is_system_header(str)) {
      return Header::system(str.substr(1, str.size() - 2));
   } else {
      for (const auto& ext : {".hpp", ".cpp", ".h", ".c"}) {
         if (os::get_file_ext(str) == ext) {
            return str;
         }
      }
      return {};
   }
}

namespace ensnare {
// Get a suitable location to store temp file.
fn temp(Str name) -> Str { return os::temp_file("ensnare_system_includes_" + name); }

// Get some verbose compiler logs to parse.
fn get_raw_include_paths(const Str& cmd_start) -> Str {
   os::write_file(temp("test"), ""); // so the compiler does not error.
   // FIXME: this is a pretty terrible check.
   Str cmd = cmd_start + " -c -v " + temp("test") + " -o " + temp("test.o") + " 2>&1";
   return os::successful_process_output(cmd);
}

fn search_paths(const Str& cmd_start) -> Vec<Str> {
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

fn ensnare::Header::search_paths() -> Vec<Str> {
   // FIXME: expose the compiler/lang as an option.
   return ensnare::search_paths("g++ -xc++");
}

fn ensnare::Header::render() const -> Str {
   return "#include " + (is_system ? "<" + name + ">" : "\"" + name + "\"") + "\n";
}

#include "ensnare/private/undef_syn.hpp"

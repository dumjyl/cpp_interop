#include "ensnare/private/config.hpp"

#include "llvm/Support/CommandLine.h"

namespace cl = llvm::cl;
using namespace ensnare;
cl::list<Str> syms("sym", cl::desc("specify specific symbols to bind. FIXME: not implimented"));
cl::list<Str> gensym_types("gensym-type", cl::desc("mangle a type symbol"));
cl::list<Str> include_dirs("include-dir", cl::desc("allow binding any headers in this directory"));
cl::opt<bool> fold_type_suffix("fold-type-suffix",
                               cl::desc("fold the inner type of a typedef with _t suffix"));
cl::opt<bool> disable_includes("disable-includes",
                               cl::desc("do not gather system includes. FIXME: not implimented"));
cl::opt<bool> ignore_const("ignore-const", cl::desc("ignore const qualifiers"));
cl::opt<Str> output(cl::Positional, cl::desc("output wrapper name/path"));
cl::list<Str> args(cl::ConsumeAfter, cl::desc("clang args..."));

const Vec<Header>& ensnare::Config::headers() const { return _headers; }
const Path& ensnare::Config::output() const { return _output; }
const Vec<Str>& ensnare::Config::user_clang_args() const { return _user_clang_args; }
const Vec<Str>& ensnare::Config::syms() const { return _syms; }
const Vec<Str>& ensnare::Config::gensym_types() const { return _gensym_types; }
const Vec<Str>& ensnare::Config::include_dirs() const { return _include_dirs; }
bool ensnare::Config::disable_includes() const { return _disable_includes; }
bool ensnare::Config::fold_type_suffix() const { return _fold_type_suffix; }
bool ensnare::Config::ignore_const() const { return _ignore_const; }

ensnare::Config::Config(int argc, const char* argv[]) {
   llvm::cl::ParseCommandLineOptions(argc, argv);
   _syms = ::syms;
   _gensym_types = ::gensym_types;
   _include_dirs = ::include_dirs;
   _fold_type_suffix = ::fold_type_suffix;
   _disable_includes = ::disable_includes;
   _ignore_const = ::ignore_const;
   _output = Str(::output);
   for (const auto& arg : args) {
      auto header = Header::parse(arg);
      if (header) {
         _headers.push_back(*header);
      } else {
         _user_clang_args.push_back(arg);
      }
   }
};

Str ensnare::Config::header_file() const {
   if (_headers.size() == 0) {
      fatal("no headers given");
   } else {
      Str result;
      for (const auto& header : _headers) {
         result += header.render();
      }
      return result;
   };
}

void ensnare::Config::dump() const {
   print("Config:");
   print("   output: ", _output);
   for (const auto& header : _headers) {
      print("   header: ", Str(header));
   }
   for (const auto& arg : _user_clang_args) {
      print("   clang arg: ", arg);
   }
   for (const auto& sym : _syms) {
      print("   sym: ", sym);
   }
}

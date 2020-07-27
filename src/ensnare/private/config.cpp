#include "ensnare/private/config.hpp"

#include "llvm/Support/CommandLine.h"

//
#include "ensnare/private/syn.hpp"

namespace cl = llvm::cl;
using namespace ensnare;
cl::list<Str> syms("sym", cl::desc("specify specific symbols to bind. FIXME: not implimented"));
cl::list<Str> include_dirs("include-dir", cl::desc("allow binding any headers in this directory"));
cl::opt<bool> fold_type_suffix("fold-type-suffix",
                               cl::desc("fold the inner type of a typedef with _t suffix"));
cl::opt<bool> disable_includes("disable-includes",
                               cl::desc("do not gather system includes. FIXME: not implimented"));
cl::opt<Str> output(cl::Positional, cl::desc("output wrapper name/path"));
cl::list<Str> args(cl::ConsumeAfter, cl::desc("clang args..."));

fn ensnare::Config::headers() const -> const Vec<Header>& { return _headers; }
fn ensnare::Config::output() const -> const Str& { return _output; }
fn ensnare::Config::user_clang_args() const -> const Vec<Str>& { return _user_clang_args; }
fn ensnare::Config::syms() const -> const Vec<Str>& { return _syms; }
fn ensnare::Config::include_dirs() const -> const Vec<Str>& { return _include_dirs; }
fn ensnare::Config::disable_includes() const -> bool { return _disable_includes; }
fn ensnare::Config::fold_type_suffix() const -> bool { return _fold_type_suffix; }

ensnare::Config::Config(int argc, const char* argv[]) {
   llvm::cl::ParseCommandLineOptions(argc, argv);
   _syms = ::syms;
   _include_dirs = ::include_dirs;
   _fold_type_suffix = ::fold_type_suffix;
   _disable_includes = ::disable_includes;
   _output = ::output;
   for (const auto& arg : args) {
      auto header = Header::parse(arg);
      if (header) {
         _headers.push_back(*header);
      } else {
         _user_clang_args.push_back(arg);
      }
   }
};

fn ensnare::Config::header_file() const -> Str {
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

fn ensnare::Config::dump() const -> void {
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

#include "ensnare/private/undef_syn.hpp"

#include "ensnare/private/config.hpp"

#include "llvm/Support/CommandLine.h"

//
#include "ensnare/private/syn.hpp"

namespace cl = llvm::cl;
using namespace ensnare;
cl::list<Str> syms("sym", cl::desc("specify specific symbols to bind"));
cl::opt<bool> disable_includes("disable-includes",
                               cl::desc("do not gather system includes. FIXME: not implimented"));
cl::opt<Str> output(cl::Positional, cl::desc("output wrapper name/path"));
cl::list<Str> args(cl::ConsumeAfter, cl::desc("clang args..."));

fn ensnare::Config::headers() const -> const Vec<Header>& { return _headers; }
fn ensnare::Config::output() const -> const Str& { return _output; }
fn ensnare::Config::user_clang_args() const -> const Vec<Str>& { return _user_clang_args; }
fn ensnare::Config::syms() const -> const Vec<Str>& { return _syms; }
fn ensnare::Config::disable_includes() const -> bool { return _disable_includes; }

ensnare::Config::Config(int argc, const char* argv[]) {
   llvm::cl::ParseCommandLineOptions(argc, argv);
   _syms = ::syms;
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

#include "ensnare/private/undef_syn.hpp"

#pragma once

#include "ensnare/private/clang_utils.hpp"
#include "ensnare/private/config.hpp"

namespace ensnare {
class HeaderCanonicalizer {
   const clang::SourceManager& source_manager;
   const Vec<Str> headers;
   using IncludeDirMap = llvm::StringMap<Str>;
   IncludeDirMap include_dir_map;
   IncludeDirMap init_include_dir_map(const Config& cfg) const;
   Vec<Str> init_headers(const Config& cfg) const;

   public:
   HeaderCanonicalizer(const Config& cfg, const clang::SourceManager& source_manager);
   Opt<Str> operator[](const clang::SourceLocation& loc);
};
} // namespace ensnare

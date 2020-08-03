#include "ensnare/private/header_canonicalizer.hpp"

#include "ensnare/private/str_utils.hpp"

// FIXME: move to os utils.
#ifdef __ARM_ARCH_ISA_A64
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

using namespace ensnare;

HeaderCanonicalizer::IncludeDirMap ensnare::HeaderCanonicalizer::init_include_dir_map(const Config& cfg) const {
   IncludeDirMap result;
   for (const auto& unprocessed_dir : cfg.include_dirs()) {
      auto dir = clang::tooling::getAbsolutePath(unprocessed_dir);
      for (const auto& entry : fs::recursive_directory_iterator(dir)) {
         if (fs::is_regular_file(entry)) {
            // FIXME: maybe we should swtich to LLVM's path handling...
            auto str_entry = Str(fs::path(entry));
            if (result.count(str_entry) == 0 || dir.size() > result[str_entry].size()) {
               result[str_entry] = dir;
            }
         }
      }
   }
   return result;
}

Vec<Str> ensnare::HeaderCanonicalizer::init_headers(const Config& cfg) const {
   Vec<Str> result;
   for (const auto& header : cfg.headers()) {
      result.push_back(header);
   }
   return result;
}

ensnare::HeaderCanonicalizer::HeaderCanonicalizer(const Config& cfg,
                                                  const clang::SourceManager& source_manager)
   : source_manager(source_manager),
     headers(init_headers(cfg)),
     include_dir_map(init_include_dir_map(cfg)) {}

Opt<Str> ensnare::HeaderCanonicalizer::operator[](const clang::SourceLocation& loc) {
   auto path = clang::tooling::getAbsolutePath(source_manager.getFilename(loc));
   for (auto const& header : headers) {
      if (ends_with(path, header)) {
         return header;
      }
   }
   if (include_dir_map.count(path) != 0) {
      return path.substr(include_dir_map[path].size() + 1);
   } else {
      return {};
   }
}

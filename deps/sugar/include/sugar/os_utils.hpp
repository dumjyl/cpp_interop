#pragma once

#include "sugar.hpp"

#include <fstream>

#if defined(__has_include)
#if __has_include(<filesystem>)
#include <filesystem>
namespace sugar {
namespace fs = std::filesystem;
}
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace sugar {
namespace fs = std::experimental::filesystem;
}
#else
#error "missing c++17 filesystem header"
#endif
#else
#error "compiler missing __has_include builtin"
#endif

namespace sugar {

using Path = fs::path;

/// Read a file of at least `bytes` bytes into `data_ptr`.
/// Return true on success.
[[nodiscard]] bool read_file(const Path& path, void* data, Size bytes);
/// Write a file at `path` from `data_ptr` which must be at least `bytes` bytes.
/// Creates any required directories.
/// Return true on success.
[[nodiscard]] bool write_file(const Path& path, void* data, Size bytes);

Opt<Str> read_file(const Path& path);

[[nodiscard]] bool write_file(const Path& path, const Str& contents);

Opt<Tuple<int, Str>> process_result(const Str& cmd, Size buf_size = 8192);

Str successful_process_output(const Str& cmd, Size buf_size = 8192);
} // namespace sugar

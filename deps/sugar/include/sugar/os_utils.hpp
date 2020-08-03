#pragma once

#include "sugar.hpp"

#include <fstream>

#ifdef __ARM_ARCH_ISA_A64
#include <experimental/filesystem>
#else
#include <filesystem>
#endif

namespace sugar {
#ifdef __ARM_ARCH_ISA_A64
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

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

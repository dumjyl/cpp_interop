#pragma once

#include "ensnare/private/utils.hpp"

//
#include "ensnare/private/syn.hpp"

namespace ensnare::os {
// Read the contents of `path` into memory.
fn read_file(const Str& path) -> Str;
// Write `contents` to disk at `path`.
fn write_file(const Str& path, const Str& contents) -> void;
// Get a suitable location
fn temp_file(const Str& name) -> Str;
// `popen` a process and return its exit code and output if process opening and closing succeeded.
// This is probably not cross platform.
fn process_result(const Str& cmd, std::size_t buf_size = 8192) -> Opt<std::tuple<int, Str>>;
// Get process output or exit the application if the process failed in any way.
fn successful_process_output(const Str& cmd, std::size_t buf_size = 8192) -> Str;
fn is_system_header(const Str& header) -> bool;
fn extract_system_header(const Str& header) -> Str;
fn is_cpp_file(const Str& header) -> bool;
fn set_file_ext(const Str& file, const Str& ext) -> Str;
} // namespace ensnare::os

#include "ensnare/private/undef_syn.hpp"

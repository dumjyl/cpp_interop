#include "ensnare/private/os_utils.hpp"

#ifdef __ARM_ARCH_ISA_A64
#include <experimental/filesystem>
#elif
#include <filesystem>
#endif
#include <fstream>

//
#include "ensnare/private/syn.hpp"

#ifdef __ARM_ARCH_ISA_A64
namespace fs = std::experimental::filesystem;
#elif
namespace fs = std::filesystem;
#endif

fn ensnare::os::read_file(const Str& path) -> Str {
   std::ifstream stream(fs::path{path});
   return Str(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

fn ensnare::os::write_file(const Str& path, const Str& contents) -> void {
   auto fs_path = fs::path(path);
   if (fs_path.parent_path() != "") {
      fs::create_directories(fs_path.parent_path());
   }
   std::ofstream stream(fs_path);
   stream << contents;
   stream.close();
}

fn ensnare::os::temp_file(const Str& name) -> Str { return fs::temp_directory_path() / name; }

fn ensnare::os::process_result(const Str& cmd, std::size_t buf_size) -> Opt<std::tuple<int, Str>> {
   auto file = popen(cmd.c_str(), "r");
   if (file == nullptr) {
      return {};
   }
   Str output;
   Str buf(buf_size, '\0');
   while (fgets(buf.data(), buf.size() - 1, file)) {
      for (auto c : buf) {
         if (c == '\0') {
            break;
         } else {
            output.push_back(c);
         }
      }
      for (auto& c : buf) {
         c = '\0';
      }
   }
   auto code = pclose(file);
   if (code == -1) {
      return {};
   }
   return std::tuple(code, output);
}

fn ensnare::os::successful_process_output(const Str& cmd, std::size_t buf_size) -> Str {
   auto process_result = os::process_result(cmd);
   if (!process_result) {
      fatal("failed to execute command: ", cmd);
   }
   auto [code, output] = *process_result;
   if (code == 0) {
      return output;
   } else {
      fatal("command exited with non-zero exit code: ", cmd);
   }
}

fn ensnare::os::set_file_ext(const Str& file, const Str& ext) -> Str {
   return fs::path(file).replace_extension(ext);
}

fn ensnare::os::get_file_ext(const Str& file) -> Str { return fs::path(file).extension(); }

#include "ensnare/private/undef_syn.hpp"

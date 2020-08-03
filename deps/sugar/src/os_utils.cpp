#include "sugar/os_utils.hpp"

namespace sugar {
[[nodiscard]] bool read_file(const Path& path, char* data, Size bytes) {
   std::ifstream file(path);
   if (file_size(path) == bytes) {
      if (file.is_open()) {
         file.read(data, bytes);
         if (file.gcount() == bytes) {
            file.close();
            return true;
         } else {
            return false;
         }
      } else {
         return false;
      }
   } else {
      return false;
   }
}

[[nodiscard]] bool write_file(const Path& path, const char* data, Size bytes) {
   fs::create_directories(path.parent_path());
   std::ofstream file(path, std::ios::binary);
   if (file.is_open()) {
      file.write(data, bytes);
      file.close();
      return true;
   } else {
      return false;
   }
}

Opt<Str> read_file(const Path& path) {
   auto bytes = file_size(path);
   if (file_size(path)) {
      Str result(bytes, '\0');
      if (read_file(path, result.data(), bytes)) {
         return result;
      }
   }
   return {};
}

[[nodiscard]] bool write_file(const Path& path, const Str& contents) {
   return write_file(path, contents.data(), contents.size());
}

Opt<Tuple<int, Str>> process_result(const Str& cmd, Size buf_size) {
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
   return Tuple<int, Str>(code, output);
}

Str successful_process_output(const Str& cmd, Size buf_size) {
   if (auto result = process_result(cmd, buf_size)) {
      auto [code, output] = *result;
      require(code == 0, "process exited with non-zero status: code = ", std::to_string(code),
              "; cmd = ", cmd);
      return output;
   } else {
      fatal("failed to execute process: ", cmd);
   }
}
} // namespace sugar

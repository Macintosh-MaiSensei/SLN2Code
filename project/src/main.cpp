/*Created by Macintosh-MaiSensei on 2026/2/18.*/
/*Version 1.0.4 Beta*/
#include "yaml2json.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <variant>
#include <vector>

#include <json.hpp>
#ifndef INCLUDE_NLOHMANN_JSON_HPP_
#include <nlohmann/json.hpp>
#endif
extern "C" {
#include <curl/curl.h>
}
#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#include <io.h>
#include <process.h>
#include <shellapi.h>
#include <tchar.h>
#include <windows.h>
#else
#include <cstdio>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;

enum class CompilerType { GCC, GXX, CLANG, CLANGXX, MSVC, UNKNOWN };

enum class DebuggerType { GDB, LLDB, CPPVSDBG, UNKNOWN };

class DirectoryNode {
public:
  std::string name;
  std::vector<DirectoryNode> children;
};

class CompilerConfig {
public:
  std::string name;
  std::string path;
  std::string cppStandard;
  std::string cStandard;
  CompilerType type = CompilerType::UNKNOWN;
  std::vector<std::string> extraArgs;
};

class DebuggerConfig {
public:
  std::string name;
  std::string path;
  DebuggerType type = DebuggerType::UNKNOWN;
};

class ProjectConfig {
public:
  std::string name;
  std::string path;
  CompilerConfig compiler;
  DebuggerConfig debugger;
  std::vector<std::string> libraries;
};

class ThirdPartyLibrary {
private:
  static std::string detectSystem() {
#ifdef _WIN32
    return "WINDOWS";
#elif __APPLE__
    return "MACOS";
#elif __linux__
    return "LINUX";
#else
    return "UNKNOWN";
#endif
  }

  static std::string detectArch() {
#ifdef _WIN64
    return "x64";
#elif __x86_64__ || __ppc64__ || _M_X64
    return "x64";
#elif __i386__ || _M_IX86
    return "x86";
#elif __aarch64__ || __arm64__
    return "ARM64";
#elif __arm__
    return "ARM";
#else
    return "UNKNOWN";
#endif
  }

  static std::string replacePlaceholders(const std::string &input,
                                         const std::string &system,
                                         const std::string &arch,
                                         const std::string &version = "") {
    std::string result = input;

    std::unordered_map<std::string, std::string> replacements = {
        {"{SYSTEM}", system}, {"{ARCH}", arch}, {"{VERSION}", version}};

    for (const auto &[placeholder, value] : replacements) {
      if (value.empty())
        continue;
      size_t pos = 0;
      while ((pos = result.find(placeholder, pos)) != std::string::npos) {
        result.replace(pos, placeholder.length(), value);
        pos += value.length();
      }
    }

    return result;
  }

public:
  std::string name;
  std::string version;
  std::string system;
  std::string arch;
  std::string downloadUrl;
  std::string includePath;
  std::string libPath;
  std::vector<std::string> dependencies;
  std::string configInstructions;

  std::unordered_map<std::string, std::string> sha256Map;

  static ThirdPartyLibrary from_json(const json &j) {
    ThirdPartyLibrary lib;
    lib.name = j.value("name", "");
    lib.version = j.value("version", "");

    lib.system = detectSystem();
    lib.arch = detectArch();

    std::string rawUrl = j.value("downloadUrl", "");
    lib.downloadUrl =
        replacePlaceholders(rawUrl, lib.system, lib.arch, lib.version);

    std::string rawIncludePath = j.value("includePath", "");
    lib.includePath =
        replacePlaceholders(rawIncludePath, lib.system, lib.arch, lib.version);

    std::string rawLibPath = j.value("libPath", "");
    lib.libPath =
        replacePlaceholders(rawLibPath, lib.system, lib.arch, lib.version);

    lib.configInstructions = j.value("configInstructions", "");

    if (j.contains("dependencies") && j["dependencies"].is_array()) {
      for (const auto &dep : j["dependencies"]) {
        lib.dependencies.push_back(dep.get<std::string>());
      }
    }

    if (j.contains("sha256") && j["sha256"].is_object()) {
      for (const auto &[key, value] : j["sha256"].items()) {
        if (value.is_string()) {
          lib.sha256Map[key] = value.get<std::string>();
        }
      }
    }

    return lib;
  }

  static ThirdPartyLibrary from_json_with_platform(const json &j,
                                                   const std::string &system,
                                                   const std::string &arch) {
    ThirdPartyLibrary lib;
    lib.name = j.value("name", "");
    lib.version = j.value("version", "");
    lib.system = system;
    lib.arch = arch;

    std::string rawUrl = j.value("downloadUrl", "");
    lib.downloadUrl = replacePlaceholders(rawUrl, system, arch, lib.version);

    std::string rawIncludePath = j.value("includePath", "");
    lib.includePath =
        replacePlaceholders(rawIncludePath, system, arch, lib.version);

    std::string rawLibPath = j.value("libPath", "");
    lib.libPath = replacePlaceholders(rawLibPath, system, arch, lib.version);

    lib.configInstructions = j.value("configInstructions", "");

    if (j.contains("dependencies") && j["dependencies"].is_array()) {
      for (const auto &dep : j["dependencies"]) {
        lib.dependencies.push_back(dep.get<std::string>());
      }
    }

    if (j.contains("sha256") && j["sha256"].is_object()) {
      for (const auto &[key, value] : j["sha256"].items()) {
        if (value.is_string()) {
          lib.sha256Map[key] = value.get<std::string>();
        }
      }
    }

    return lib;
  }

  std::string getSha256() const {
    std::string key = system + "-" + arch;
    auto it = sha256Map.find(key);
    if (it != sha256Map.end()) {
      return it->second;
    }
    return "";
  }

  std::string getSha256ForPlatform(const std::string &targetSystem,
                                   const std::string &targetArch) const {
    std::string key = targetSystem + "-" + targetArch;
    auto it = sha256Map.find(key);
    if (it != sha256Map.end()) {
      return it->second;
    }
    return "";
  }

  std::unordered_map<std::string, std::string> getAllSha256() const {
    return sha256Map;
  }

  json to_json() const {
    json result = {{"name", name},
                   {"version", version},
                   {"system", system},
                   {"arch", arch},
                   {"downloadUrl", downloadUrl},
                   {"includePath", includePath},
                   {"libPath", libPath},
                   {"dependencies", dependencies},
                   {"configInstructions", configInstructions}};

    json sha256Json;
    for (const auto &[key, value] : sha256Map) {
      sha256Json[key] = value;
    }
    result["sha256"] = sha256Json;

    return result;
  }

  static std::string getCurrentSystem() { return detectSystem(); }

  static std::string getCurrentArch() { return detectArch(); }

  bool isCurrentPlatformSupported() const {
    std::string key = system + "-" + arch;
    return sha256Map.find(key) != sha256Map.end();
  }
};

namespace Constants {
const std::string VERSION = "1.0.4";
const std::string DEFAULT_PROJECT_NAME = "project";
const std::string DEFAULT_LIBRARY_MIRROR = "";
const std::string LIBRARY_INFO_FILENAME = "libraries";

const std::unordered_map<std::string, CompilerType> COMPILER_TYPE_MAP = {
    {"gcc", CompilerType::GCC},     {"g++", CompilerType::GXX},
    {"clang", CompilerType::CLANG}, {"clang++", CompilerType::CLANGXX},
    {"cl", CompilerType::MSVC},     {"msvc", CompilerType::MSVC}};

const std::unordered_map<std::string, DebuggerType> DEBUGGER_TYPE_MAP = {
    {"gdb", DebuggerType::GDB},
    {"lldb", DebuggerType::LLDB},
    {"cppvsdbg", DebuggerType::CPPVSDBG}};
} // namespace Constants

class SHA256 {
private:
  static constexpr std::array<uint32_t, 64> K = {
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
      0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
      0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
      0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
      0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
      0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
      0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
      0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
      0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
      0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
      0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

  std::array<uint32_t, 8> state = {0x6a09e667, 0xbb67ae85, 0x3c6ef372,
                                   0xa54ff53a, 0x510e527f, 0x9b05688c,
                                   0x1f83d9ab, 0x5be0cd19};

  static uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
  }

  static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
  }

  static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
  }

  static uint32_t sigma0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
  }

  static uint32_t sigma1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
  }

  static uint32_t gamma0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
  }

  static uint32_t gamma1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
  }

  void transform(const uint8_t *data) {
    std::array<uint32_t, 64> W = {0};

    for (int i = 0; i < 16; i++) {
      W[i] = (static_cast<uint32_t>(data[i * 4]) << 24) |
             (static_cast<uint32_t>(data[i * 4 + 1]) << 16) |
             (static_cast<uint32_t>(data[i * 4 + 2]) << 8) |
             (static_cast<uint32_t>(data[i * 4 + 3]));
    }

    for (int i = 16; i < 64; i++) {
      W[i] = gamma1(W[i - 2]) + W[i - 7] + gamma0(W[i - 15]) + W[i - 16];
    }

    auto a = state[0];
    auto b = state[1];
    auto c = state[2];
    auto d = state[3];
    auto e = state[4];
    auto f = state[5];
    auto g = state[6];
    auto h = state[7];

    for (int i = 0; i < 64; i++) {
      uint32_t T1 = h + sigma1(e) + ch(e, f, g) + K[i] + W[i];
      uint32_t T2 = sigma0(a) + maj(a, b, c);
      h = g;
      g = f;
      f = e;
      e = d + T1;
      d = c;
      c = b;
      b = a;
      a = T1 + T2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
  }

public:
  static std::string hash_file(const fs::path &file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
      throw std::runtime_error("Can't open the file: " + file_path.string());
    }

    SHA256 sha;
    std::array<uint8_t, 64> buffer = {0};
    uint64_t total_bytes = 0;
    bool processed_last_block = false;

    while (!processed_last_block) {
      file.read(reinterpret_cast<char *>(buffer.data()), 64);
      size_t bytes_read = file.gcount();
      total_bytes += bytes_read;

      if (bytes_read < 64) {

        buffer[bytes_read] = 0x80;

        if (bytes_read >= 56) {

          for (size_t i = bytes_read + 1; i < 64; i++) {
            buffer[i] = 0;
          }
          sha.transform(buffer.data());

          buffer.fill(0);

          uint64_t bit_length = total_bytes * 8;
          for (int i = 0; i < 8; i++) {
            buffer[63 - i] = static_cast<uint8_t>(bit_length >> (i * 8));
          }
          sha.transform(buffer.data());
        } else {

          for (size_t i = bytes_read + 1; i < 56; i++) {
            buffer[i] = 0;
          }

          uint64_t bit_length = total_bytes * 8;
          for (int i = 0; i < 8; i++) {
            buffer[63 - i] = static_cast<uint8_t>(bit_length >> (i * 8));
          }
          sha.transform(buffer.data());
        }
        processed_last_block = true;
      } else {

        sha.transform(buffer.data());
      }
    }

    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (uint32_t val : sha.state) {
      result << std::setw(8) << val;
    }

    return result.str();
  }
};

class SafeCommandExecutor {
public:
  static std::string execute(const std::vector<std::string> &args) {
    return CommandService::execute(args);
  }

  static bool download_file(const std::string &url,
                            const fs::path &output_path) {
    return CommandService::download_file(url, output_path);
  }

  static bool unzip_file(const fs::path &zip_file, const fs::path &output_dir) {
    return CommandService::unzip_file(zip_file, output_dir);
  }

  static bool safe_create_directory(const fs::path &path) {
    return FileUtils::safe_create_directory(path);
  }

  static bool is_valid_url(const std::string &url) {
    return FileUtils::is_valid_url(url);
  }

private:
  struct FileUtils {
    static bool safe_create_directory(const fs::path &path) {
      try {
        if (!fs::exists(path)) {
          std::error_code ec;
          bool created = fs::create_directories(path, ec);
          if (!created || ec) {
            std::cerr << "Failed to create directory " << path << ": "
                      << ec.message() << std::endl;
            return false;
          }
        }
        return true;
      } catch (const fs::filesystem_error &e) {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
        return false;
      }
    }

    static bool is_valid_url(const std::string &url) {
      try {
        if (url.find("http://") != 0 && url.find("https://") != 0) {
          return false;
        }

        for (char c : url) {
          if (std::isspace(c) || c < 0x20) {
            return false;
          }
        }

        if (url.find("..") != std::string::npos ||
            url.find(";") != std::string::npos ||
            url.find("|") != std::string::npos ||
            url.find("`") != std::string::npos ||
            url.find("$") != std::string::npos ||
            url.find("(") != std::string::npos ||
            url.find(")") != std::string::npos) {
          return false;
        }

        if (url.find("https://github.com/") == 0 ||
            url.find("https://archives.boost.io/") == 0 ||
            url.find("https://www.libsdl.org/") == 0) {
          return true;
        }

        static const std::regex domain_regex(
            R"(^(?:[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?\.)+[a-zA-Z]{2,}$)");

        size_t start = url.find("://") + 3;
        size_t end = url.find('/', start);
        if (end == std::string::npos) {
          end = url.length();
        }

        std::string domain = url.substr(start, end - start);
        size_t port_pos = domain.find(':');
        if (port_pos != std::string::npos) {
          domain = domain.substr(0, port_pos);
        }

        return std::regex_match(domain, domain_regex);
      } catch (const std::exception &e) {
        std::cerr << "URL validation error: " << e.what() << "\n";
        return false;
      }
    }
  };

  struct LibcurlDownloader {
  private:
    static size_t write_data(void *ptr, size_t size, size_t nmemb,
                             FILE *stream) {
      size_t written = fwrite(ptr, size, nmemb, stream);
      return written;
    }

    static int progress_callback(void *clientp, curl_off_t dltotal,
                                 curl_off_t dlnow, curl_off_t ultotal,
                                 curl_off_t ulnow) {
      if (dltotal > 0) {
        double percent = static_cast<double>(dlnow) / dltotal * 100.0;
        std::cout << "\rDownload progress: " << static_cast<int>(percent)
                  << "% (" << dlnow << "/" << dltotal << " bytes)";
        std::cout.flush();
      }
      return 0;
    }

  public:
    static bool download(const std::string &url, const fs::path &output_path) {
      CURL *curl = curl_easy_init();
      if (!curl) {
        std::cerr << "Failed to initialize libcurl" << std::endl;
        return false;
      }

      FILE *fp = fopen(output_path.string().c_str(), "wb");
      if (!fp) {
        std::cerr << "Failed to open file for writing: " << output_path
                  << std::endl;
        curl_easy_cleanup(curl);
        return false;
      }

      bool success = false;

      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
      curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
      curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "SafeCommandExecutor/1.0");

      CURLcode res = curl_easy_perform(curl);

      if (res == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (response_code >= 200 && response_code < 300) {
          curl_off_t dl_size;
          curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &dl_size);
          std::cout << "\nDownload successful. Downloaded " << dl_size
                    << " bytes" << std::endl;
          success = true;
        } else {
          std::cerr << "\nHTTP error: " << response_code << std::endl;
        }
      } else {
        std::cerr << "\nDownload failed: " << curl_easy_strerror(res)
                  << std::endl;
      }

      fclose(fp);
      curl_easy_cleanup(curl);

      if (!success && fs::exists(output_path)) {
        std::error_code ec;
        fs::remove(output_path, ec);
        if (ec) {
          std::cerr << "Failed to remove partial download: " << ec.message()
                    << std::endl;
        }
      }

      return success;
    }
  };

  struct IPlatformExecutor {
    virtual ~IPlatformExecutor() = default;
    virtual std::string execute(const std::vector<std::string> &args) = 0;
    virtual bool is_command_available(const std::string &command) = 0;
    virtual bool is_safe_argument(const std::string &arg) = 0;
  };

#ifdef _WIN32
  struct WindowsExecutor : public IPlatformExecutor {
    std::string execute(const std::vector<std::string> &args) override {
      if (args.empty()) {
        throw std::invalid_argument("No command to execute");
      }

      std::string command_line;
      for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0)
          command_line += " ";
        if (args[i].find(' ') != std::string::npos ||
            args[i].find('\t') != std::string::npos) {
          command_line += "\"" + args[i] + "\"";
        } else {
          command_line += args[i];
        }
      }
      command_line += " 2>&1";

      std::array<char, 128> buffer;
      std::string result;
      FILE *pipe = _popen(command_line.c_str(), "r");

      if (!pipe) {
        throw std::runtime_error("_popen() failed!");
      }

      try {
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) !=
               nullptr) {
          result += buffer.data();
        }
      } catch (...) {
        _pclose(pipe);
        throw;
      }

      int return_code = _pclose(pipe);
      if (return_code != 0) {
        throw std::runtime_error(
            "Command failed with exit code: " + std::to_string(return_code) +
            "\nOutput: " + result);
      }

      return result;
    }

    bool is_command_available(const std::string &command) override {
      std::string check_cmd = "where " + command + " >nul 2>nul";
      int result = std::system(check_cmd.c_str());
      return result == 0;
    }

    bool is_safe_argument(const std::string &arg) override {
      if (arg.empty())
        return false;
      const std::string dangerous_chars = ";&|`$><!*?{}[]()^%";
      for (char c : dangerous_chars) {
        if (arg.find(c) != std::string::npos)
          return false;
      }
      if (arg.find("..") != std::string::npos)
        return false;
      if (arg.find('\n') != std::string::npos ||
          arg.find('\r') != std::string::npos)
        return false;
      return true;
    }
  };
#endif

#ifndef _WIN32
  struct PosixExecutor : public IPlatformExecutor {
    std::string execute(const std::vector<std::string> &args) override {
      if (args.empty()) {
        throw std::invalid_argument("No command to execute");
      }

      std::vector<char *> argv;
      for (const auto &arg : args) {
        argv.push_back(const_cast<char *>(arg.c_str()));
      }
      argv.push_back(nullptr);

      int pipefd[2];
      if (pipe(pipefd) == -1) {
        throw std::runtime_error("Failed to create pipe: " +
                                 std::string(strerror(errno)));
      }

      pid_t pid = fork();
      if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        throw std::runtime_error("Failed to fork process: " +
                                 std::string(strerror(errno)));
      }

      if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
          perror("dup2 stdout");
          exit(EXIT_FAILURE);
        }
        if (dup2(pipefd[1], STDERR_FILENO) == -1) {
          perror("dup2 stderr");
          exit(EXIT_FAILURE);
        }
        close(pipefd[1]);

        if (setgid(getgid()) != 0 || setuid(getuid()) != 0) {
          perror("setgid/setuid");
          exit(EXIT_FAILURE);
        }

        execvp(argv[0], argv.data());
        std::cerr << "Failed to execute command: " << argv[0] << " - "
                  << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
      }

      close(pipefd[1]);
      std::string result;
      char buffer[4096];
      ssize_t count;

      while ((count = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[count] = '\0';
        result.append(buffer, count);
      }

      if (count == -1 && errno != EINTR) {
        close(pipefd[0]);
        throw std::runtime_error("Error reading from pipe: " +
                                 std::string(strerror(errno)));
      }

      close(pipefd[0]);

      int status;
      do {
        if (waitpid(pid, &status, 0) == -1) {
          if (errno == EINTR)
            continue;
          throw std::runtime_error("waitpid failed: " +
                                   std::string(strerror(errno)));
        }
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));

      if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        throw std::runtime_error("Command exited with non-zero status: " +
                                 std::to_string(WEXITSTATUS(status)) +
                                 "\nOutput: " + result);
      }

      if (WIFSIGNALED(status)) {
        throw std::runtime_error("Command terminated by signal: " +
                                 std::to_string(WTERMSIG(status)));
      }

      return result;
    }

    bool is_command_available(const std::string &command) override {
      std::string check_cmd = "which " + command + " >/dev/null 2>&1";
      int result = std::system(check_cmd.c_str());
      return result == 0;
    }

    bool is_safe_argument(const std::string &arg) override {
      if (arg.empty())
        return false;
      const std::string dangerous_chars = ";&|`$><!*?{}[]()~";
      for (char c : dangerous_chars) {
        if (arg.find(c) != std::string::npos)
          return false;
      }
      if (arg.find("..") != std::string::npos)
        return false;
      if (arg.find('\n') != std::string::npos ||
          arg.find('\r') != std::string::npos)
        return false;
      return true;
    }
  };
#endif

  struct ExecutorFactory {
    static IPlatformExecutor *create() {
#ifdef _WIN32
      return new WindowsExecutor();
#else
      return new PosixExecutor();
#endif
    }
  };

  struct CommandService {
    static std::string execute(const std::vector<std::string> &args) {
      if (args.empty()) {
        throw std::invalid_argument("Command arguments cannot be empty");
      }

      std::unique_ptr<IPlatformExecutor> executor(ExecutorFactory::create());

      for (const auto &arg : args) {
        if (!executor->is_safe_argument(arg)) {
          throw std::runtime_error("Unsafe argument detected: " + arg);
        }
      }

      std::string command_str;
      for (const auto &arg : args) {
        command_str += arg + " ";
      }
      std::cout << "Executing safe command: " << command_str << std::endl;

      return executor->execute(args);
    }

    static bool download_file(const std::string &url,
                              const fs::path &output_path) {
      std::unique_ptr<IPlatformExecutor> executor(ExecutorFactory::create());

      if (!FileUtils::is_valid_url(url)) {
        throw std::invalid_argument("Invalid URL format: " + url);
      }

      if (output_path.empty() || output_path.is_relative()) {
        throw std::invalid_argument("Output path must be absolute");
      }

      if (output_path.string().find("..") != std::string::npos) {
        throw std::invalid_argument(
            "Output path contains parent directory traversal");
      }

      if (!FileUtils::safe_create_directory(output_path.parent_path())) {
        throw std::runtime_error("Failed to create output directory");
      }

      std::cout << "Downloading from: " << url << std::endl;
      std::cout << "Saving to: " << output_path << std::endl;

      bool success = LibcurlDownloader::download(url, output_path);

      if (success) {
        if (fs::exists(output_path)) {
          std::error_code ec;
          auto file_size = fs::file_size(output_path, ec);
          if (!ec && file_size > 0) {
            std::cout << "Download completed successfully. File size: "
                      << file_size << " bytes" << std::endl;
            return true;
          } else if (ec) {
            std::cerr << "Failed to get file size: " << ec.message()
                      << std::endl;
            return false;
          } else if (file_size == 0) {
            std::cerr << "Downloaded file is empty" << std::endl;
            return false;
          }
        }
      }

      return false;
    }

    static bool unzip_file(const fs::path &zip_file,
                           const fs::path &output_dir) {
      std::unique_ptr<IPlatformExecutor> executor(ExecutorFactory::create());

      if (!fs::exists(zip_file) || !fs::is_regular_file(zip_file)) {
        throw std::invalid_argument("Invalid zip file: " + zip_file.string());
      }

      if (!FileUtils::safe_create_directory(output_dir)) {
        throw std::invalid_argument("Failed to create output directory: " +
                                    output_dir.string());
      }

      if (zip_file.string().find("..") != std::string::npos ||
          output_dir.string().find("..") != std::string::npos) {
        throw std::invalid_argument("Path contains parent directory traversal");
      }

      std::vector<std::string> args;
#ifdef _WIN32
      if (!executor->is_command_available("powershell")) {
        throw std::runtime_error("powershell command is not available.");
      }

      args = {
          "powershell",      "-Command",         "Expand-Archive",    "-Path",
          zip_file.string(), "-DestinationPath", output_dir.string(), "-Force"};
#else
      if (!executor->is_command_available("unzip")) {
        throw std::runtime_error(
            "unzip command is not available. Please install unzip first.");
      }

      args = {"unzip", "-o", zip_file.string(), "-d", output_dir.string()};
#endif

      try {
        std::string result = execute(args);

        bool has_files = false;
        for (const auto &entry : fs::directory_iterator(output_dir)) {
          has_files = true;
          break;
        }

        if (has_files) {
          std::cout << "Unzip successful. Files extracted to: " << output_dir
                    << std::endl;
          return true;
        } else {
          std::cerr << "Unzip completed but no files found in output directory"
                    << std::endl;
          return false;
        }

      } catch (const std::exception &e) {
        std::cerr << "Unzip failed: " << e.what() << std::endl;
        return false;
      }
    }
  };
};

class Utils {
public:
  static std::string clean_path(const std::string &input) {
    std::string cleaned = input;

    cleaned.erase(0, cleaned.find_first_not_of(" \t\r\n"));
    cleaned.erase(cleaned.find_last_not_of(" \t\r\n") + 1);

    while (!cleaned.empty() &&
           (cleaned.front() == '\'' || cleaned.front() == '"'))
      cleaned.erase(0, 1);
    while (!cleaned.empty() &&
           (cleaned.back() == '\'' || cleaned.back() == '"'))
      cleaned.pop_back();

    std::replace(cleaned.begin(), cleaned.end(), '\\', '/');

    if (!cleaned.empty() && cleaned.back() == '/' && cleaned.size() > 1) {
      cleaned.pop_back();
    }

    return cleaned;
  }

  static std::string get_valid_name(const std::string &input) {
    if (input.find_first_of(";&|<>`$()") != std::string::npos) {
      throw std::invalid_argument("Invalid characters in project name");
    }
    std::string name = input;

    name.erase(std::remove_if(name.begin(), name.end(),
                              [](char c) {
                                return !(std::isalnum(c) || c == '-' ||
                                         c == '_' || c == ' ');
                              }),
               name.end());

    std::replace(name.begin(), name.end(), ' ', '-');

    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return name;
  }

  static bool is_standard_supported(const CompilerConfig &compiler,
                                    const std::string &standard) {
    static std::mutex cache_mutex;
    static std::map<std::tuple<CompilerType, std::string, int, int, int>, bool>
        cache;

    if (compiler.type == CompilerType::UNKNOWN) {
      return true;
    }

    std::string s = standard;
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (s == "c++latest" || s == "clatest") {
      return true;
    }

    try {
      auto probe = [&compiler]() -> std::tuple<int, int, int> {
        std::string exe =
            !compiler.path.empty() ? compiler.path : compiler.name;
        if (exe.empty())
          return {-1, -1, -1};

        std::vector<std::vector<std::string>> cmds = {
            {exe, "--version"}, {exe, "-v"}, {exe, "-dumpversion"}, {exe}};

        for (auto &cmd : cmds) {
          try {
            std::string out = Utils::safe_execute_command(cmd);
            if (out.empty())
              continue;

            std::regex ver_re(R"((\d+)\.(\d+)(?:\.(\d+))?)");
            std::smatch m;
            if (std::regex_search(out, m, ver_re)) {
              int major = std::stoi(m[1].str());
              int minor = std::stoi(m[2].str());
              int patch = 0;
              if (m.size() > 3 && m[3].matched)
                patch = std::stoi(m[3].str());
              return {major, minor, patch};
            }
          } catch (...) {
          }
        }
        return {-1, -1, -1};
      };

      auto [maj, min, pat] = probe();

      if (maj < 0)
        return true;

      auto cache_key = std::make_tuple(compiler.type, s, maj, min, pat);
      {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = cache.find(cache_key);
        if (it != cache.end()) {
          return it->second;
        }
      }

      auto at_least = [&](int M, int m = 0) {
        if (maj > M)
          return true;
        if (maj < M)
          return false;
        return min >= m;
      };

      bool supported = true;

      if (s.rfind("c++", 0) == 0) {
        if (s == "c++98" || s == "c++03")
          supported = true;
        else if (s == "c++11") {
          if (compiler.type == CompilerType::GCC ||
              compiler.type == CompilerType::GXX)
            supported = (maj > 4) || (maj == 4 && min >= 8);
          else if (compiler.type == CompilerType::CLANG ||
                   compiler.type == CompilerType::CLANGXX)
            supported = at_least(3, 3);
          else if (compiler.type == CompilerType::MSVC)
            supported = maj >= 19;
        } else if (s == "c++14") {
          if (compiler.type == CompilerType::GCC ||
              compiler.type == CompilerType::GXX)
            supported = at_least(5, 0);
          else if (compiler.type == CompilerType::CLANG ||
                   compiler.type == CompilerType::CLANGXX)
            supported = at_least(3, 4);
          else if (compiler.type == CompilerType::MSVC)
            supported = maj >= 19;
        } else if (s == "c++17") {
          if (compiler.type == CompilerType::GCC ||
              compiler.type == CompilerType::GXX)
            supported = at_least(7, 0);
          else if (compiler.type == CompilerType::CLANG ||
                   compiler.type == CompilerType::CLANGXX)
            supported = at_least(5, 0);
          else if (compiler.type == CompilerType::MSVC)
            supported = (maj == 19 && min >= 10) || (maj > 19);
        } else if (s == "c++20") {
          if (compiler.type == CompilerType::GCC ||
              compiler.type == CompilerType::GXX)
            supported = at_least(10, 0);
          else if (compiler.type == CompilerType::CLANG ||
                   compiler.type == CompilerType::CLANGXX)
            supported = at_least(11, 0);
          else if (compiler.type == CompilerType::MSVC)
            supported = (maj == 19 && min >= 25) || (maj > 19);
        } else if (s == "c++23") {
          if (compiler.type == CompilerType::GCC ||
              compiler.type == CompilerType::GXX)
            supported = at_least(12, 0);
          else if (compiler.type == CompilerType::CLANG ||
                   compiler.type == CompilerType::CLANGXX)
            supported = at_least(13, 0);
          else if (compiler.type == CompilerType::MSVC)
            supported = (maj == 19 && min >= 30) || (maj > 19);
        }
      } else if (s.rfind("c", 0) == 0) {
        if (s == "c99")
          supported = true;
        else if (s == "c11") {
          if (compiler.type == CompilerType::GCC ||
              compiler.type == CompilerType::GXX)
            supported = at_least(5, 0);
          else if (compiler.type == CompilerType::CLANG ||
                   compiler.type == CompilerType::CLANGXX)
            supported = at_least(3, 4);
          else if (compiler.type == CompilerType::MSVC)
            supported = maj >= 19;
        } else if (s == "c17" || s == "c18")
          supported = true;
        else if (s == "c20" || s == "c23") {
          if (compiler.type == CompilerType::GCC ||
              compiler.type == CompilerType::GXX)
            supported = at_least(11, 0);
          else if (compiler.type == CompilerType::CLANG ||
                   compiler.type == CompilerType::CLANGXX)
            supported = at_least(11, 0);
          else if (compiler.type == CompilerType::MSVC)
            supported = (maj == 19 && min >= 30) || (maj > 19);
        }
      }

      {
        std::lock_guard<std::mutex> lock(cache_mutex);
        cache[cache_key] = supported;
      }

      return supported;

    } catch (...) {
      return true;
    }
  }

  static int execute_command(const std::string &command) {
    std::cout << "Executing: " << command << std::endl;
    return std::system(command.c_str());
  }

  static std::string
  safe_execute_command(const std::vector<std::string> &args) {
    return SafeCommandExecutor::execute(args);
  }

  static bool safe_download_file(const std::string &url,
                                 const fs::path &output_path) {
    return SafeCommandExecutor::download_file(url, output_path);
  }

  static bool safe_unzip_file(const fs::path &zip_file,
                              const fs::path &output_dir) {
    return SafeCommandExecutor::unzip_file(zip_file, output_dir);
  }

  static bool safe_create_directory(const fs::path &path) {
    return SafeCommandExecutor::safe_create_directory(path);
  }

  static fs::path join_paths(const fs::path &base, const std::string &part) {
    return base / part;
  }

  static bool safe_write_file(const fs::path &path,
                              const std::string &content) {
    try {
      std::ofstream file(path);
      if (!file)
        return false;
      file << content;
      return true;
    } catch (const std::exception &e) {
      std::cerr << "Error writing file: " << e.what() << std::endl;
      return false;
    }
  }

  static std::string safe_read_file(const fs::path &path) {
    try {
      std::ifstream file(path);
      if (!file)
        return "";
      std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
      return content;
    } catch (const std::exception &e) {
      std::cerr << "Error reading file: " << e.what() << std::endl;
      return "";
    }
  }

  static std::string trim(const std::string &str) {
    auto start = std::find_if_not(str.begin(), str.end(),
                                  [](int c) { return std::isspace(c); });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](int c) {
                 return std::isspace(c);
               }).base();
    return (start < end) ? std::string(start, end) : std::string();
  }

  static json parse_json(const std::string &json_str) {
    try {
      return json::parse(json_str);
    } catch (const json::exception &e) {
      std::cerr << "JSON parse error: " << e.what() << std::endl;
      return json();
    }
  }

  static json read_json_file(const fs::path &path) {
    try {
      std::ifstream file(path);
      if (!file) {
        return json();
      }
      return json::parse(file);
    } catch (const json::exception &e) {
      std::cerr << "JSON file read error: " << e.what() << std::endl;
      return json();
    }
  }
  static json read_yaml_file(const fs::path &path) {
    try {
      YAML::Node yaml = YAML::LoadFile(path.string());
      return yaml2json::convert(yaml);
    } catch (const YAML::Exception &e) {
      std::cerr << "YAML file read error: " << e.what() << std::endl;
      return json();
    }
  }
  static json parse_yaml(const std::string &yaml_str) {
    try {
      YAML::Node yaml = YAML::Load(yaml_str);
      return yaml2json::convert(yaml);
    } catch (const YAML::Exception &e) {
      std::cerr << "YAML parse error: " << e.what() << std::endl;
      return json();
    }
  }
  static bool write_json_file(const fs::path &path, const json &j) {
    try {
      std::ofstream file(path);
      if (!file) {
        return false;
      }
      file << j.dump(2);
      return true;
    } catch (const std::exception &e) {
      std::cerr << "JSON file write error: " << e.what() << std::endl;
      return false;
    }
  }
  static json read_config_file(const fs::path &path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   ::tolower);

    if (extension == ".yaml" || extension == ".yml") {
      return read_yaml_file(path);
    } else {
      return read_json_file(path);
    }
  }
};

class ILibraryInfoProvider {
public:
  virtual ~ILibraryInfoProvider() = default;

  virtual std::vector<std::string> get_available_libraries() = 0;

  virtual std::optional<ThirdPartyLibrary>
  get_library_info(const std::string &lib_name) = 0;

  virtual bool refresh_library_info() = 0;

  virtual std::string get_provider_name() const = 0;

  virtual fs::path get_library_info_path() const = 0;
};

class MultiFormatLibraryProvider : public ILibraryInfoProvider {
private:
  fs::path library_info_path_;
  std::string download_url_;
  std::unordered_map<std::string, ThirdPartyLibrary> libraries_;
  mutable std::mutex cache_mutex_;

public:
  MultiFormatLibraryProvider(const fs::path &info_path,
                             const std::string &url = "")
      : library_info_path_(info_path), download_url_(url) {

    refresh_library_info();
  }

  std::vector<std::string> get_available_libraries() override {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    std::vector<std::string> libs;
    for (const auto &pair : libraries_) {
      libs.push_back(pair.first);
    }
    return libs;
  }

  std::optional<ThirdPartyLibrary>
  get_library_info(const std::string &lib_name) override {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = libraries_.find(lib_name);
    if (it != libraries_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  bool refresh_library_info() override {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (!fs::exists(library_info_path_)) {
      if (!download_url_.empty()) {
        if (!download_library_info()) {
          std::cerr << "Failed to download library info from: " << download_url_
                    << std::endl;
          return false;
        }
      } else {
        std::cout
            << "Library info file not found and no download URL provided.\n";
        return false;
      }
    }

    std::string extension = library_info_path_.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   ::tolower);

    json j;
    if (extension == ".yaml" || extension == ".yml") {
      j = Utils::read_yaml_file(library_info_path_);
      if (j.is_null()) {
        std::cerr << "Invalid library info YAML file: " << library_info_path_
                  << std::endl;
        return false;
      }
    } else {
      j = Utils::read_json_file(library_info_path_);
      if (j.is_null() || !j.is_object()) {
        std::cerr << "Invalid library info JSON file: " << library_info_path_
                  << std::endl;
        return false;
      }
    }

    libraries_.clear();
    for (auto it = j.begin(); it != j.end(); ++it) {
      try {
        ThirdPartyLibrary lib = ThirdPartyLibrary::from_json(it.value());
        libraries_[it.key()] = lib;
      } catch (const std::exception &e) {
        std::cerr << "Error parsing library info for " << it.key() << ": "
                  << e.what() << std::endl;
      }
    }

    std::cout << "Loaded " << libraries_.size()
              << " libraries from: " << library_info_path_ << std::endl;
    return true;
  }

  std::string get_provider_name() const override {
    return "Multi-Format Library Provider (" + library_info_path_.string() +
           ")";
  }

  fs::path get_library_info_path() const override { return library_info_path_; }

private:
  bool download_library_info() {
    std::cout << "Downloading library info from: " << download_url_
              << std::endl;

    fs::path temp_path = library_info_path_;
    temp_path.replace_extension(".tmp");

    if (!SafeCommandExecutor::download_file(download_url_, temp_path)) {
      return false;
    }

    json j = Utils::read_json_file(temp_path);
    if (j.is_null() || !j.is_object()) {
      try {
        fs::remove(temp_path);
      } catch (...) {
      }
      std::cerr << "Downloaded file is not valid JSON" << std::endl;
      return false;
    }

    try {
      if (fs::exists(library_info_path_)) {
        fs::remove(library_info_path_);
      }
      fs::rename(temp_path, library_info_path_);
    } catch (const fs::filesystem_error &e) {
      std::cerr << "Error moving library info file: " << e.what() << std::endl;
      return false;
    }

    std::cout << "Library info downloaded successfully to: "
              << library_info_path_ << std::endl;
    return true;
  }
};

class JsonLibraryProvider : public ILibraryInfoProvider {
private:
  fs::path library_info_path_;
  std::string download_url_;
  std::unordered_map<std::string, ThirdPartyLibrary> libraries_;
  mutable std::mutex cache_mutex_;

public:
  JsonLibraryProvider(const fs::path &info_path, const std::string &url = "")
      : library_info_path_(info_path), download_url_(url) {
    refresh_library_info();
  }

  std::vector<std::string> get_available_libraries() override {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    std::vector<std::string> libs;
    for (const auto &pair : libraries_) {
      libs.push_back(pair.first);
    }
    return libs;
  }

  std::optional<ThirdPartyLibrary>
  get_library_info(const std::string &lib_name) override {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = libraries_.find(lib_name);
    if (it != libraries_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  bool refresh_library_info() override {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (!fs::exists(library_info_path_)) {
      if (!download_url_.empty()) {
        if (!download_library_info()) {
          std::cerr << "Failed to download library info from: " << download_url_
                    << std::endl;
          return false;
        }
      } else {
        std::cout
            << "Library info file not found and no download URL provided.\n";
        return false;
      }
    }

    json j = Utils::read_json_file(library_info_path_);
    if (j.is_null() || !j.is_object()) {
      std::cerr << "Invalid library info JSON file: " << library_info_path_
                << std::endl;
      return false;
    }

    libraries_.clear();
    for (auto it = j.begin(); it != j.end(); ++it) {
      try {
        ThirdPartyLibrary lib = ThirdPartyLibrary::from_json(it.value());
        libraries_[it.key()] = lib;
      } catch (const std::exception &e) {
        std::cerr << "Error parsing library info for " << it.key() << ": "
                  << e.what() << std::endl;
      }
    }

    std::cout << "Loaded " << libraries_.size()
              << " libraries from: " << library_info_path_ << std::endl;
    return true;
  }

  std::string get_provider_name() const override {
    return "JSON Library Provider (" + library_info_path_.string() + ")";
  }

  fs::path get_library_info_path() const override { return library_info_path_; }

private:
  bool download_library_info() {
    std::cout << "Downloading library info from: " << download_url_
              << std::endl;

    fs::path temp_path = library_info_path_;
    temp_path.replace_extension(".tmp");

    if (!SafeCommandExecutor::download_file(download_url_, temp_path)) {
      return false;
    }

    json j = Utils::read_json_file(temp_path);
    if (j.is_null() || !j.is_object()) {
      fs::remove(temp_path);
      std::cerr << "Downloaded file is not valid JSON" << std::endl;
      return false;
    }

    try {
      if (fs::exists(library_info_path_)) {
        fs::remove(library_info_path_);
      }
      fs::rename(temp_path, library_info_path_);
    } catch (const fs::filesystem_error &e) {
      std::cerr << "Error moving library info file: " << e.what() << std::endl;
      return false;
    }

    std::cout << "Library info downloaded successfully to: "
              << library_info_path_ << std::endl;
    return true;
  }
};

class CompilerService {
public:
  static CompilerConfig get_compiler_config() {
    CompilerConfig config;

    std::cout << "\n=== Compiler Configuration ===\n";

    std::cout << "Select compiler type:\n";
    std::cout << "1. gcc\n";
    std::cout << "2. g++\n";
    std::cout << "3. clang\n";
    std::cout << "4. clang++\n";
    std::cout << "5. MSVC (Visual Studio)\n";
    std::cout << "Enter choice (1-5, default 1): ";

    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "2") {
      config.type = CompilerType::GXX;
      config.name = "g++";
    } else if (choice == "3") {
      config.type = CompilerType::CLANG;
      config.name = "clang";
    } else if (choice == "4") {
      config.type = CompilerType::CLANGXX;
      config.name = "clang++";
    } else if (choice == "5") {
      config.type = CompilerType::MSVC;
      config.name = "cl";
    } else {
      config.type = CompilerType::GXX;
      config.name = "g++";
    }

    config.path = find_compiler_path(config);

    config.cppStandard = select_cpp_standard(config);

    config.cStandard = select_c_standard(config);

    std::cout << "Enter any additional compiler flags (space separated, or "
                 "press Enter for none): ";
    std::string extra_flags;
    std::getline(std::cin, extra_flags);

    if (!extra_flags.empty()) {
      std::istringstream iss(extra_flags);
      std::string flag;
      while (iss >> flag) {
        config.extraArgs.push_back(flag);
      }
    }

    return config;
  }

private:
  static std::string find_compiler_path(CompilerConfig &config) {
    std::vector<std::string> compiler_paths;

    if (config.type == CompilerType::MSVC) {
      compiler_paths = find_msvc_paths();
    } else {
      compiler_paths = find_compiler_in_path(config.name);
    }

    if (compiler_paths.empty()) {
      std::cout << "No " << config.name << " compiler found.\n";
      std::cout << "Enter " << config.name << " compiler path: ";
      std::string path;
      std::getline(std::cin, path);
      return Utils::clean_path(path);
    }

    if (compiler_paths.size() == 1) {
      std::cout << "Using " << config.name
                << " compiler found at: " << compiler_paths[0] << "\n";
      return compiler_paths[0];
    }

    std::cout << "Multiple " << config.name << " compilers found:\n";
    for (size_t i = 0; i < compiler_paths.size(); ++i) {
      std::cout << "  " << i + 1 << ". " << compiler_paths[i] << "\n";
    }
    std::cout << "Select compiler (1-" << compiler_paths.size() << "): ";
    std::string selection;
    std::getline(std::cin, selection);

    try {
      size_t idx = std::stoi(selection) - 1;
      if (idx < compiler_paths.size()) {
        return compiler_paths[idx];
      }
    } catch (...) {
    }

    return compiler_paths[0];
  }

  static std::vector<std::string>
  find_compiler_in_path(const std::string &executable_name) {
    std::vector<std::string> found_paths;

    const char *path_env = std::getenv("PATH");
    if (!path_env) {
      return found_paths;
    }

    std::string path_str(path_env);

#ifdef _WIN32
    const char delimiter = ';';
    std::string exe_name = executable_name + ".exe";
#else
    const char delimiter = ':';
    std::string exe_name = executable_name;
#endif

    std::istringstream path_stream(path_str);
    std::string directory;

    while (std::getline(path_stream, directory, delimiter)) {
      fs::path dir_path(directory);
      if (fs::exists(dir_path) && fs::is_directory(dir_path)) {
        fs::path executable_path = dir_path / exe_name;
        if (fs::exists(executable_path) &&
            fs::is_regular_file(executable_path)) {
          found_paths.push_back(Utils::clean_path(executable_path.string()));
        }
      }
    }

    return found_paths;
  }

  static std::vector<std::string> find_msvc_paths() {
    std::vector<std::string> paths;

#ifdef _WIN32

    std::vector<std::string> common_paths = {
        "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC",
        "C:/Program Files/Microsoft Visual "
        "Studio/2022/Professional/VC/Tools/MSVC",
        "C:/Program Files/Microsoft Visual "
        "Studio/2022/Enterprise/VC/Tools/MSVC",
        "C:/Program Files (x86)/Microsoft Visual "
        "Studio/2019/Community/VC/Tools/MSVC",
        "C:/Program Files (x86)/Microsoft Visual "
        "Studio/2019/Professional/VC/Tools/MSVC",
        "C:/Program Files (x86)/Microsoft Visual "
        "Studio/2019/Enterprise/VC/Tools/MSVC"};

    for (const auto &base_path : common_paths) {
      if (fs::exists(base_path)) {
        for (const auto &entry : fs::directory_iterator(base_path)) {
          if (entry.is_directory()) {
            fs::path compiler_path = entry.path() / "bin/Hostx64/x64/cl.exe";
            if (fs::exists(compiler_path)) {
              paths.push_back(Utils::clean_path(compiler_path.string()));
            }
          }
        }
      }
    }
#endif

    return paths;
  }

  static std::string select_cpp_standard(const CompilerConfig &compiler) {
    std::cout << "Select C++ standard:\n";
    std::cout << "1. C++98\n";
    std::cout << "2. C++11\n";
    std::cout << "3. C++14\n";
    std::cout << "4. C++17\n";
    std::cout << "5. C++20\n";
    std::cout << "6. C++23\n";
    std::cout << "Enter choice (1-6, default 4): ";

    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "1" && Utils::is_standard_supported(compiler, "c++98")) {
      return "c++98";
    } else if (choice == "2" &&
               Utils::is_standard_supported(compiler, "c++11")) {
      return "c++11";
    } else if (choice == "3" &&
               Utils::is_standard_supported(compiler, "c++14")) {
      return "c++14";
    } else if (choice == "5" &&
               Utils::is_standard_supported(compiler, "c++20")) {
      return "c++20";
    } else if (choice == "6" &&
               Utils::is_standard_supported(compiler, "c++23")) {
      return "c++23";
    }
    return "c++17";
  }

  static std::string select_c_standard(const CompilerConfig &compiler) {
    std::cout << "Select C standard:\n";
    std::cout << "1. C99\n";
    std::cout << "2. C11\n";
    std::cout << "3. C17\n";
    std::cout << "4. C20\n";
    std::cout << "5. C23\n";
    std::cout << "Enter choice (1-5, default 3): ";

    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "1" && Utils::is_standard_supported(compiler, "c99")) {
      return "c99";
    } else if (choice == "2" && Utils::is_standard_supported(compiler, "c11")) {
      return "c11";
    } else if (choice == "4" && Utils::is_standard_supported(compiler, "c20")) {
      return "c20";
    } else if (choice == "5" && Utils::is_standard_supported(compiler, "c23")) {
      return "c23";
    }
    return "c17";
  }
};

class DebuggerService {
public:
  static DebuggerConfig get_debugger_config() {
    DebuggerConfig config;

    std::cout << "\n=== Debugger Configuration ===\n";

    std::cout << "Select debugger type:\n";
    std::cout << "1. GDB\n";
    std::cout << "2. LLDB\n";
    std::cout << "3. Visual Studio Debugger\n";
    std::cout << "Enter choice (1-3, default 1): ";

    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "2") {
      config.type = DebuggerType::LLDB;
      config.name = "lldb";
    } else if (choice == "3") {
      config.type = DebuggerType::CPPVSDBG;
      config.name = "Visual Studio Debugger";
    } else {
      config.type = DebuggerType::GDB;
      config.name = "gdb";
    }

    config.path = find_debugger_path(config);

    return config;
  }

private:
  static std::string find_debugger_path(DebuggerConfig &config) {
    std::vector<std::string> debugger_paths;

    if (config.type == DebuggerType::CPPVSDBG) {
      debugger_paths = find_vs_debugger_paths();
    } else {
      debugger_paths = find_debugger_in_path(config.name);
    }

    if (debugger_paths.empty()) {
      std::cout << "No " << config.name << " debugger found.\n";
      std::cout << "Enter " << config.name << " debugger path: ";
      std::string path;
      std::getline(std::cin, path);
      return Utils::clean_path(path);
    }

    if (debugger_paths.size() == 1) {
      std::cout << "Using " << config.name
                << " debugger found at: " << debugger_paths[0] << "\n";
      return debugger_paths[0];
    }

    std::cout << "Multiple " << config.name << " debuggers found:\n";
    for (size_t i = 0; i < debugger_paths.size(); ++i) {
      std::cout << "  " << i + 1 << ". " << debugger_paths[i] << "\n";
    }
    std::cout << "Select debugger (1-" << debugger_paths.size() << "): ";
    std::string selection;
    std::getline(std::cin, selection);

    try {
      size_t idx = std::stoi(selection) - 1;
      if (idx < debugger_paths.size()) {
        return debugger_paths[idx];
      }
    } catch (...) {
    }

    return debugger_paths[0];
  }

  static std::vector<std::string>
  find_debugger_in_path(const std::string &executable_name) {
    std::vector<std::string> found_paths;

    const char *path_env = std::getenv("PATH");
    if (!path_env) {
      return found_paths;
    }

    std::string path_str(path_env);

#ifdef _WIN32
    const char delimiter = ';';
    std::string exe_name = executable_name + ".exe";
#else
    const char delimiter = ':';
    std::string exe_name = executable_name;
#endif

    std::istringstream path_stream(path_str);
    std::string directory;

    while (std::getline(path_stream, directory, delimiter)) {
      fs::path dir_path(directory);
      if (fs::exists(dir_path) && fs::is_directory(dir_path)) {
        fs::path executable_path = dir_path / exe_name;
        if (fs::exists(executable_path) &&
            fs::is_regular_file(executable_path)) {
          found_paths.push_back(Utils::clean_path(executable_path.string()));
        }
      }
    }

    return found_paths;
  }

  static std::vector<std::string> find_vs_debugger_paths() {
    std::vector<std::string> paths;

#ifdef _WIN32

    std::vector<std::string> common_paths = {
        "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE",
        "C:/Program Files/Microsoft Visual "
        "Studio/2022/Professional/Common7/IDE",
        "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE",
        "C:/Program Files (x86)/Microsoft Visual "
        "Studio/2019/Community/Common7/IDE",
        "C:/Program Files (x86)/Microsoft Visual "
        "Studio/2019/Professional/Common7/IDE",
        "C:/Program Files (x86)/Microsoft Visual "
        "Studio/2019/Enterprise/Common7/IDE"};

    for (const auto &path : common_paths) {
      fs::path debugger_path = fs::path(path) / "Debugger/vsdbg.exe";
      if (fs::exists(debugger_path)) {
        paths.push_back(Utils::clean_path(debugger_path.string()));
      }
    }
#endif

    return paths;
  }
};

class ProjectStructureService {
public:
  static DirectoryNode get_project_structure(const std::string &project_name) {
    return {
        project_name,
        {{".vscode", {}},
         {"include", {}},
         {"lib", {}},
         {"lib64", {}},
         {"src", {}},
         {"build", {{"bin", {{"Debug", {}}, {"Release", {}}}}, {"obj", {}}}},
         {"third_party", {}},
         {"docs", {}}}};
  }

  static bool use_unicode_symbols;

#if defined(_WIN32)
  static bool create_directory_recursive(const fs::path &base_path,
                                         const DirectoryNode &node,
                                         int depth = 0,
                                         const std::string &prefix = "") {
    const std::string vertical_line = "|";
    const std::string branch = "|-- ";
    const std::string last_branch = "`-- ";
    const std::string space_fill = "    ";
    const std::string success_mark = " [OK]";
    const std::string fail_mark = " [FAIL]";
    const std::string exist_mark = " [EXIST]";

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    WORD originalAttrs = consoleInfo.wAttributes;

    bool result = create_directory_recursive_impl(
        base_path, node, depth, prefix, vertical_line, branch, last_branch,
        space_fill, success_mark, fail_mark, exist_mark);

    SetConsoleTextAttribute(hConsole, originalAttrs);
    return result;
  }
#else
  static bool create_directory_recursive(const fs::path &base_path,
                                         const DirectoryNode &node,
                                         int depth = 0,
                                         const std::string &prefix = "") {

    if (use_unicode_symbols) {
      const std::string vertical_line = "│";
      const std::string branch = "├── ";
      const std::string last_branch = "└── ";
      const std::string space_fill = "    ";
      const std::string success_mark = "\033[32m ✓\033[0m";
      const std::string fail_mark = "\033[31m ✗\033[0m";
      const std::string exist_mark = "\033[32m ✓\033[0m";

      return create_directory_recursive_impl(
          base_path, node, depth, prefix, vertical_line, branch, last_branch,
          space_fill, success_mark, fail_mark, exist_mark);
    } else {
      const std::string vertical_line = "|";
      const std::string branch = "|-- ";
      const std::string last_branch = "`-- ";
      const std::string space_fill = "    ";
      const std::string success_mark = " [OK]";
      const std::string fail_mark = " [FAIL]";
      const std::string exist_mark = " [EXIST]";

      return create_directory_recursive_impl(
          base_path, node, depth, prefix, vertical_line, branch, last_branch,
          space_fill, success_mark, fail_mark, exist_mark);
    }
  }
#endif

private:
  static bool create_directory_recursive_impl(
      const fs::path &base_path, const DirectoryNode &node, int depth,
      const std::string &prefix, const std::string &vertical_line,
      const std::string &branch, const std::string &last_branch,
      const std::string &space_fill, const std::string &success_mark,
      const std::string &fail_mark, const std::string &exist_mark) {

    const fs::path current_path = Utils::join_paths(base_path, node.name);
    std::string tree_prefix;

    if (depth > 0) {
      tree_prefix = prefix + (depth > 1 ? vertical_line + "   " : "") +
                    (depth > 0 ? branch : "");
    }

    std::cout << tree_prefix << node.name;

    try {
      if (!fs::exists(current_path)) {
        if (Utils::safe_create_directory(current_path)) {
          std::cout << success_mark << "\n";
        } else {
          std::cout << fail_mark << "\n";
          return false;
        }
      } else {
        std::cout << exist_mark << "\n";
      }

      std::string child_prefix =
          prefix + (depth > 0 ? vertical_line + "   " : "");
      for (size_t i = 0; i < node.children.size(); i++) {
        const auto &child = node.children[i];
        bool is_last = (i == node.children.size() - 1);

        std::string new_prefix = is_last ? space_fill : vertical_line + "   ";
        std::string new_branch = is_last ? last_branch : branch;

        if (!create_directory_recursive_impl(
                current_path, child, depth + 1, child_prefix + new_branch,
                vertical_line, branch, last_branch, space_fill, success_mark,
                fail_mark, exist_mark)) {
          return false;
        }
      }

      return true;
    } catch (const fs::filesystem_error &e) {
      std::cout << " [ERROR] " << e.what() << "\n";
      return false;
    }
  }
};
#ifdef _WIN32
bool ProjectStructureService::use_unicode_symbols = false;
#else
bool ProjectStructureService::use_unicode_symbols = true;
#endif
class LibraryService {
private:
  static std::unique_ptr<ILibraryInfoProvider> provider_;

  static fs::path get_default_library_info_path() {

    std::vector<std::string> extensions = {".json", ".yaml", ".yml"};

    for (const auto &ext : extensions) {
      fs::path local_path =
          fs::current_path() / (Constants::LIBRARY_INFO_FILENAME + ext);
      if (fs::exists(local_path)) {
        return local_path;
      }
    }

    const char *home_dir = std::getenv("HOME");
    if (home_dir) {
      for (const auto &ext : extensions) {
        fs::path home_path = fs::path(home_dir) / ".sln2code" /
                             (Constants::LIBRARY_INFO_FILENAME + ext);
        if (fs::exists(home_path)) {
          return home_path;
        }
      }
    }

    return fs::current_path() / (Constants::LIBRARY_INFO_FILENAME + ".json");
  }

  static ILibraryInfoProvider &get_provider() {
    if (!provider_) {

      fs::path local_info_path = get_default_library_info_path();
      provider_ = std::make_unique<MultiFormatLibraryProvider>(local_info_path);
    }
    return *provider_;
  }

public:
  static bool configure_library_mirror(const std::string &mirror_url) {
    if (mirror_url.empty()) {
      std::cout << "Clearing library mirror configuration.\n";
      provider_.reset();
      return true;
    }

    if (!SafeCommandExecutor::is_valid_url(mirror_url)) {
      std::cerr << "Invalid mirror URL: " << mirror_url << std::endl;
      return false;
    }

    std::cout << "Configuring library mirror: " << mirror_url << std::endl;

    fs::path local_cache =
        fs::temp_directory_path() / "sln2code_libraries.json";
    auto remote_provider =
        std::make_unique<MultiFormatLibraryProvider>(local_cache, mirror_url);

    if (!remote_provider->refresh_library_info()) {
      std::cerr << "Failed to connect to library mirror: " << mirror_url
                << std::endl;
      return false;
    }

    provider_ = std::move(remote_provider);
    std::cout << "Library mirror configured successfully. Found "
              << provider_->get_available_libraries().size() << " libraries.\n";
    return true;
  }

  static bool auto_detect_library_info() {
    fs::path detected_path = find_library_info_file();
    if (detected_path.empty()) {
      std::cout << "No library info file found in common locations.\n";
      return false;
    }

    std::cout << "Detected library info file: " << detected_path << std::endl;
    provider_ = std::make_unique<MultiFormatLibraryProvider>(detected_path);
    return provider_->refresh_library_info();
  }

  static fs::path find_library_info_file() {
    std::vector<std::string> filenames = {
        Constants::LIBRARY_INFO_FILENAME + ".json",
        Constants::LIBRARY_INFO_FILENAME + ".yaml",
        Constants::LIBRARY_INFO_FILENAME + ".yml",
        "libraries.json",
        "libraries.yaml",
        "libraries.yml"};

    for (const auto &filename : filenames) {
      fs::path path = fs::current_path() / filename;
      if (fs::exists(path)) {
        return path;
      }
    }

    const char *home_dir = std::getenv("HOME");
    if (home_dir) {
      fs::path home_path = fs::path(home_dir) / ".sln2code";
      for (const auto &filename : filenames) {
        fs::path path = home_path / filename;
        if (fs::exists(path)) {
          return path;
        }
      }
    }

    return fs::path();
  }

  static bool set_library_info_path(const fs::path &path) {
    if (!fs::exists(path)) {
      std::cerr << "Library info file not found: " << path << std::endl;
      return false;
    }

    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   ::tolower);

    std::vector<std::string> supported_formats = {".json", ".yaml", ".yml"};
    if (std::find(supported_formats.begin(), supported_formats.end(),
                  extension) == supported_formats.end()) {
      std::cerr << "Unsupported file format: " << extension << std::endl;
      return false;
    }

    provider_ = std::make_unique<MultiFormatLibraryProvider>(path);
    return provider_->refresh_library_info();
  }

  static void offer_library_installation(const fs::path &project_path) {
    std::cout << "\nWould you like to add any third-party libraries? [y/N]: ";
    std::string response;
    std::getline(std::cin, response);

    if (response.empty() || (response[0] != 'y' && response[0] != 'Y')) {
      return;
    }

    auto &provider = get_provider();
    auto available_libs = provider.get_available_libraries();

    if (available_libs.empty()) {
      std::cout << "No libraries available from the current provider.\n";

      std::cout << "Would you like to configure a library mirror? [y/N]: ";
      std::string mirror_response;
      std::getline(std::cin, mirror_response);

      if (!mirror_response.empty() &&
          (mirror_response[0] == 'y' || mirror_response[0] == 'Y')) {
        std::cout << "Enter library mirror URL (or press Enter to skip): ";
        std::string mirror_url;
        std::getline(std::cin, mirror_url);

        if (!mirror_url.empty()) {
          configure_library_mirror(mirror_url);

          available_libs = provider.get_available_libraries();
        }
      }

      if (available_libs.empty()) {
        return;
      }
    }

    while (true) {
      std::cout << "Enter library name (or 'done' to finish, 'list' to show "
                   "available): ";
      std::string lib_input;
      std::getline(std::cin, lib_input);

      if (lib_input == "done") {
        break;
      }

      if (lib_input == "list") {
        std::cout << "Available libraries:\n";
        for (size_t i = 0; i < available_libs.size(); ++i) {
          std::cout << "  " << i + 1 << ". " << available_libs[i] << "\n";
        }
        continue;
      }

      std::string lib_name = lib_input;
      if (lib_name.empty())
        continue;

      add_third_party_library(project_path, lib_name);
    }
  }

  static bool add_third_party_library(const fs::path &project_path,
                                      const std::string &lib_name) {
    static std::set<std::string> installed_libs;
    static std::mutex mtx;

    {
      std::lock_guard<std::mutex> lock(mtx);
      if (installed_libs.find(lib_name) != installed_libs.end()) {
        std::cout << lib_name << " already installed. Skipping.\n";
        return true;
      }
      installed_libs.insert(lib_name);
    }

    const fs::path third_party_dir = project_path / "third_party";
    Utils::safe_create_directory(third_party_dir);

    auto &provider = get_provider();
    auto lib_info = provider.get_library_info(lib_name);

    if (!lib_info) {
      std::cerr << "Library not found: " << lib_name << std::endl;

      auto available_libs = provider.get_available_libraries();
      if (!available_libs.empty()) {
        std::cout << "Available libraries: ";
        for (size_t i = 0; i < available_libs.size(); ++i) {
          if (i > 0)
            std::cout << ", ";
          std::cout << available_libs[i];
        }
        std::cout << "\n";
      }
      return false;
    }

    std::cout << "\nAdding " << lib_info->name << " to project...\n";

    fs::path lib_dir = project_path / "third_party" / lib_info->name;
    if (fs::exists(lib_dir)) {
      std::cout << "Library directory already exists. Skipping download.\n";
    } else {

      if (!download_and_extract_library(project_path, *lib_info)) {
        std::cerr << "Failed to download and extract " << lib_info->name
                  << std::endl;
        return false;
      }
    }

    auto future_guide = std::async(std::launch::async, [&]() {
      generate_library_guide(project_path, *lib_info);
    });

    auto future_cmake = std::async(std::launch::async, [&]() {
      update_cmake_with_library(project_path, *lib_info);
    });

    for (const auto &dep : lib_info->dependencies) {
      std::cout << "Installing dependency: " << dep << "\n";
      if (!add_third_party_library(project_path, dep)) {
        std::cerr << "Failed to install dependency: " << dep << std::endl;
        return false;
      }
    }

    future_guide.get();
    future_cmake.get();

    std::cout << lib_info->name << " added successfully!\n";
    std::cout << "See docs/" << lib_info->name
              << "_GUIDE.md for usage instructions\n";

    return true;
  }

  static bool download_and_extract_library(const fs::path &project_path,
                                           const ThirdPartyLibrary &lib) {
    const fs::path third_party_dir = project_path / "third_party";
    const fs::path zip_file = third_party_dir / (lib.name + ".zip");

    std::cout << "\nDownloading " << lib.name << " from " << lib.downloadUrl
              << "...\n";

    if (!SafeCommandExecutor::download_file(lib.downloadUrl, zip_file)) {
      std::cerr << "Failed to download " << lib.name << std::endl;
      return false;
    }

    std::string expected_sha = lib.getSha256();
    if (!expected_sha.empty()) {
      std::cout << "Verifying SHA256 checksum...\n";
      try {
        std::string calculated_sha = SHA256::hash_file(zip_file);
        if (calculated_sha != expected_sha) {
          std::cerr << "SHA256 verification failed!\n";
          std::cerr << "Expected: " << expected_sha << "\n";
          std::cerr << "Actual:   " << calculated_sha << "\n";
          fs::remove(zip_file);
          return false;
        }
        std::cout << "SHA256 verification passed.\n";
      } catch (const std::exception &e) {
        std::cerr << "Error during SHA256 calculation: " << e.what()
                  << std::endl;
        fs::remove(zip_file);
        return false;
      }
    }

    std::cout << "Extracting " << lib.name << "...\n";

    if (!SafeCommandExecutor::unzip_file(zip_file, third_party_dir)) {
      std::cerr << "Failed to extract " << lib.name << std::endl;
      return false;
    }

    std::thread([zip_file]() {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      try {
        if (fs::exists(zip_file)) {
          fs::remove(zip_file);
          std::cout << "Cleaned up temporary zip file.\n";
        }
      } catch (const std::exception &e) {
        std::cerr << "Error removing zip file: " << e.what() << std::endl;
      }
    }).detach();

    return true;
  }

  static void generate_library_guide(const fs::path &project_path,
                                     const ThirdPartyLibrary &lib) {
    const fs::path docs_dir = project_path / "docs";
    Utils::safe_create_directory(docs_dir);

    const fs::path guide_path = docs_dir / (lib.name + "_GUIDE.md");

    std::string content = "# " + lib.name + " Integration Guide\n\n";
    content += "## Installation\n";
    content += "The library has been downloaded to: `third_party/" +
               fs::path(lib.downloadUrl).stem().string() + "`\n\n";
    content += "## Configuration\n";
    content += "### CMake Configuration\n";
    content += "```cmake\n" + lib.configInstructions + "\n```\n\n";
    content += "### Include in Code\n";

    if (lib.name == "GLFW") {
      content += "```cpp\n#include <GLFW/glfw3.h>\n```\n\n";
    } else if (lib.name == "Boost") {
      content += "```cpp\n#include <boost/filesystem.hpp>\n```\n\n";
    } else if (lib.name == "SDL2") {
      content += "```cpp\n#include <SDL.h>\n```\n\n";
    } else {
      content += "```cpp\n#include <" + lib.name + ".h>\n```\n\n";
    }

    content += "## Official Documentation\n";
    if (lib.name == "GLFW") {
      content += "- [GLFW Documentation](链接5)\n";
    } else if (lib.name == "Boost") {
      content += "- [Boost Documentation](链接6)\n";
    } else if (lib.name == "SDL2") {
      content += "- [SDL2 Documentation](链接2)\n";
    } else {
      content += "- Check the official website for " + lib.name + "\n";
    }

    Utils::safe_write_file(guide_path, content);
  }

  static void update_cmake_with_library(const fs::path &project_path,
                                        const ThirdPartyLibrary &lib) {
    const fs::path cmake_path = project_path / "CMakeLists.txt";
    if (!fs::exists(cmake_path)) {
      return;
    }

    std::string current_content = Utils::safe_read_file(cmake_path);
    if (current_content.find(lib.name) != std::string::npos) {
      std::cout << "Library " << lib.name
                << " already configured in CMakeLists.txt\n";
      return;
    }

    std::ofstream cmake(cmake_path, std::ios::app);
    if (cmake) {
      cmake << "\n# " << lib.name << " Configuration\n";
      cmake << lib.configInstructions << "\n";
      std::cout << "Updated CMakeLists.txt with " << lib.name
                << " configuration\n";
    }
  }

  static void list_available_libraries() {
    auto &provider = get_provider();
    auto libraries = provider.get_available_libraries();

    std::cout << "\nAvailable libraries (" << libraries.size() << "):\n";
    for (const auto &lib_name : libraries) {
      auto lib_info = provider.get_library_info(lib_name);
      if (lib_info) {
        std::cout << "  " << lib_name << " - " << lib_info->downloadUrl << "\n";
      } else {
        std::cout << "  " << lib_name << " - [Info not available]\n";
      }
    }
  }

  static bool refresh_library_info() {
    auto &provider = get_provider();
    std::cout << "Refreshing library information from: "
              << provider.get_provider_name() << std::endl;
    return provider.refresh_library_info();
  }

  static fs::path get_library_info_path() {
    auto &provider = get_provider();
    return provider.get_library_info_path();
  }
};

std::unique_ptr<ILibraryInfoProvider> LibraryService::provider_ = nullptr;

class ProjectGenerator {
public:
  static void generate_project_files(const fs::path &project_path,
                                     const std::string &project_name,
                                     const CompilerConfig &compiler,
                                     const DebuggerConfig &debugger) {
    const fs::path vscode_dir = project_path / ".vscode";
    Utils::safe_create_directory(vscode_dir);

    std::vector<std::future<void>> futures;

    futures.push_back(std::async(std::launch::async, [&]() {
      generate_c_cpp_properties(vscode_dir, compiler);
    }));

    futures.push_back(std::async(std::launch::async, [&]() {
      generate_tasks_json(vscode_dir, project_name, compiler);
    }));

    futures.push_back(std::async(std::launch::async, [&]() {
      generate_launch_json(vscode_dir, project_name, compiler, debugger);
    }));

    futures.push_back(std::async(
        std::launch::async, [&]() { generate_settings_json(vscode_dir); }));

    futures.push_back(std::async(std::launch::async, [&]() {
      generate_cmake_file(project_path, project_name, compiler);
    }));

    futures.push_back(std::async(std::launch::async, [&]() {
      generate_main_cpp(project_path, project_name);
    }));

    futures.push_back(std::async(std::launch::async,
                                 [&]() { generate_gitignore(project_path); }));

    futures.push_back(std::async(std::launch::async, [&]() {
      generate_readme(project_path, project_name);
    }));

    for (auto &future : futures) {
      future.get();
    }
  }

private:
  static void generate_c_cpp_properties(const fs::path &vscode_dir,
                                        const CompilerConfig &compiler) {
    std::string intelliSenseMode = get_intellisense_mode(compiler.type);
    std::string compilerPath = Utils::clean_path(compiler.path);

    std::string content = R"({
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/include",
                "${workspaceFolder}/third_party/**"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "compilerPath": ")" +
                          compilerPath + R"(",
            "cStandard": ")" +
                          compiler.cStandard + R"(",
            "cppStandard": ")" +
                          compiler.cppStandard + R"(",
            "intelliSenseMode": ")" +
                          intelliSenseMode + R"("
        },
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/include",
                "${workspaceFolder}/third_party/**"
            ],
            "defines": [],
            "compilerPath": ")" +
                          compilerPath + R"(",
            "cStandard": ")" +
                          compiler.cStandard + R"(",
            "cppStandard": ")" +
                          compiler.cppStandard + R"(",
            "intelliSenseMode": ")" +
                          intelliSenseMode + R"("
        }
    ],
    "version": 4
})";

    Utils::safe_write_file(vscode_dir / "c_cpp_properties.json", content);
  }

  static std::string get_intellisense_mode(CompilerType type) {

    std::string platform;
    std::string architecture;

#if defined(_WIN32) || defined(_WIN64)
    platform = "windows";
#if defined(_M_AMD64) || defined(__x86_64__)
    architecture = "x64";
#elif defined(_M_IX86)
    architecture = "x86";
#elif defined(_M_ARM)
    architecture = "arm";
#elif defined(_M_ARM64) || defined(__aarch64__)
    architecture = "arm64";
#else
    architecture = "x64";
#endif
#elif defined(__APPLE__)
    platform = "macos";
#if defined(__x86_64__)
    architecture = "x64";
#elif defined(__aarch64__) || defined(__arm64__)
    architecture = "arm64";
#else
    architecture = "arm64";
#endif
#elif defined(__linux__)
    platform = "linux";
#if defined(__x86_64__)
    architecture = "x64";
#elif defined(__i386__)
    architecture = "x86";
#elif defined(__aarch64__)
    architecture = "arm64";
#elif defined(__arm__)
    architecture = "arm";
#else
    architecture = "x64";
#endif
#else
    platform = "linux";
    architecture = "x64";
#endif

    switch (type) {
    case CompilerType::CLANG:
    case CompilerType::CLANGXX:
      return platform + "-clang-" + architecture;

    case CompilerType::MSVC:
      return platform + "-msvc-" + architecture;

    case CompilerType::GCC:
    case CompilerType::GXX:
    default:
      return platform + "-gcc-" + architecture;
    }
  }

  static void generate_tasks_json(const fs::path &vscode_dir,
                                  const std::string &project_name,
                                  const CompilerConfig &compiler) {
    std::string output_ext =
        (compiler.type == CompilerType::MSVC) ? ".exe" : "";
    std::string compilerPath = Utils::clean_path(compiler.path);

    std::string args;
    if (compiler.type == CompilerType::MSVC) {
      args =
          R"(                "/std:)" + compiler.cppStandard.substr(2) + R"(",
                "/I${workspaceFolder}/include",
                "/I${workspaceFolder}/third_party",)";
    } else {
      args = R"(                "-std=)" + compiler.cppStandard + R"(",
                "-I${workspaceFolder}/include",
                "-I${workspaceFolder}/third_party",)";
    }

    for (const auto &arg : compiler.extraArgs) {
      args += "\n                \"" + arg + "\",";
    }

    args += R"(
                "-L${workspaceFolder}/third_party",)";

    std::string problemMatcher =
        (compiler.type == CompilerType::MSVC) ? R"("$msCompile")" : R"("$gcc")";

    std::string content = R"({
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build",
            "type": "shell",
            "command": ")" +
                          compilerPath + R"(",
            "args": [
)" + args + R"(
                "${workspaceFolder}/src/*.cpp",
                "-o",
                "${workspaceFolder}/build/bin/Debug/)" +
                          project_name + output_ext + R"("
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": [)" +
                          problemMatcher + R"(],
            "detail": "Built with )" +
                          compiler.name + R"("
        },
        {
            "label": "Clean",
            "type": "shell",
            "command": "rm",
            "args": [
                "-rf",
                "${workspaceFolder}/build/bin/Debug/*"
            ]
        }
    ]
})";

    Utils::safe_write_file(vscode_dir / "tasks.json", content);
  }

  static void generate_launch_json(const fs::path &vscode_dir,
                                   const std::string &project_name,
                                   const CompilerConfig &compiler,
                                   const DebuggerConfig &debugger) {
    std::string output_ext =
        (compiler.type == CompilerType::MSVC) ? ".exe" : "";
    std::string debuggerPath = Utils::clean_path(debugger.path);
    std::string debuggerType;

    switch (debugger.type) {
    case DebuggerType::GDB:
      debuggerType = "gdb";
      break;
    case DebuggerType::LLDB:
      debuggerType = "lldb";
      break;
    case DebuggerType::CPPVSDBG:
      debuggerType = "cppvsdbg";
      break;
    default:
      debuggerType = "cppdbg";
    }

    std::string content =
        R"({
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug )" +
        project_name + R"(",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/bin/Debug/)" +
        project_name + output_ext + R"(",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": )" +
        (compiler.type == CompilerType::MSVC ? "true" : "false") + R"(,
            "MIMode": ")" +
        debuggerType + R"(",
            "miDebuggerPath": ")" +
        debuggerPath + R"(",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "Build",
            "logging": {
                "moduleLoad": false,
                "trace": false
            }
        }
    ]
})";

    Utils::safe_write_file(vscode_dir / "launch.json", content);
  }

  static void generate_settings_json(const fs::path &vscode_dir) {
    std::string content = R"({
    "files.associations": {
        "*.h": "c",
        "*.hpp": "cpp",
        "*.ipp": "cpp"
    },
    "editor.formatOnSave": true,
    "C_Cpp.default.configurationProvider": "ms-vscode.cpptools",
    "explorer.confirmDragAndDrop": false,
    "cmake.configureOnOpen": false
})";

    Utils::safe_write_file(vscode_dir / "settings.json", content);
  }

  static void generate_cmake_file(const fs::path &project_path,
                                  const std::string &project_name,
                                  const CompilerConfig &compiler) {
    std::string content = R"(cmake_minimum_required(VERSION 3.20)
project()" + project_name +
                          R"( VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD )" +
                          compiler.cppStandard.substr(2) + R"()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set C standard
set(CMAKE_C_STANDARD )" + compiler.cStandard.substr(1) +
                          R"()
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build configuration
set(CMAKE_BUILD_TYPE Debug)

# Include directories
include_directories(include)
include_directories(third_party)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")

# Create executable
add_executable()" + project_name +
                          R"( ${SOURCES})

# Compiler options
target_compile_options()" +
                          project_name + R"( PRIVATE)";

    for (const auto &arg : compiler.extraArgs) {
      content += "\n    " + arg;
    }

    content += R"()

# Installation
install(TARGETS )" +
               project_name + R"( DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)

# Third-party libraries configuration will be added here automatically
)";

    Utils::safe_write_file(project_path / "CMakeLists.txt", content);
  }

  static void generate_main_cpp(const fs::path &project_path,
                                const std::string &project_name) {
    const fs::path src_dir = project_path / "src";
    Utils::safe_create_directory(src_dir);

    std::string content = R"(#include <iostream>

int main() {
    std::cout << "Hello, )" +
                          project_name + R"(!\n";
    std::cout << "Project created successfully!\n";
    std::cout << "Build with: cmake -B build && cmake --build build\n";
    return 0;
}
)";

    Utils::safe_write_file(src_dir / "main.cpp", content);
  }

  static void generate_gitignore(const fs::path &project_path) {
    std::string content = R"(# Build artifacts
build/
*.exe
*.out
*.o
*.obj
*.a
*.lib
*.dll
*.so
*.dylib

# IDE files
.vscode/
!.vscode/tasks.json
!.vscode/launch.json
!.vscode/c_cpp_properties.json
!.vscode/settings.json

# System files
.DS_Store
Thumbs.db

# Temporary files
*.tmp
*.temp

# Log files
*.log
)";

    Utils::safe_write_file(project_path / ".gitignore", content);
  }

  static void generate_readme(const fs::path &project_path,
                              const std::string &project_name) {
    std::string content = R"(# )" + project_name + R"(

A C++ project created with SLN2Code.

## Build Instructions

### Using CMake (Recommended)
```bash
mkdir build
cd build
cmake ..
```make
cmake --build .
```
## Development
Open the project in VS Code and press F5 to build and debug.
## Project Structure
- `src/`: Source files.
- `include/`: Header files.
- `lib/`: 32-bit libraries.
- `lib64/`: 64-bit libraries.
- `build/`: Build artifacts.
- `third_party/`: Third-party libraries.
- `docs/`: Documentation files.
## License
This project is licensed under the MIT License.
)";
    Utils::safe_write_file(project_path / "README.md", content);
  }
};

class CommandLineParser {
public:
  struct Options {
    std::string project_name = Constants::DEFAULT_PROJECT_NAME;
    std::string base_path = fs::current_path().string();
    std::vector<std::string> libraries_to_install;
    std::string library_mirror_url;
    CompilerConfig compiler;
    DebuggerConfig debugger;
    bool show_version = false;
    bool show_help = false;
    bool list_libraries = false;
    bool refresh_libraries = false;
  };

  static Options parse(int argc, char *argv[]) {
    Options options;

    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];

      if (arg == "--name" || arg == "-n") {
        if (i + 1 < argc) {
          options.project_name = Utils::get_valid_name(argv[++i]);
        } else {
          throw std::runtime_error("Missing project name after " + arg);
        }
      } else if (arg == "--path" || arg == "-p") {
        if (i + 1 < argc) {
          options.base_path = Utils::clean_path(argv[++i]);
        } else {
          throw std::runtime_error("Missing path after " + arg);
        }
      } else if (arg == "-v" || arg == "--version") {
        options.show_version = true;
      } else if (arg == "-I" || arg == "--install") {
        if (i + 1 < argc) {
          options.libraries_to_install.push_back(argv[++i]);
        } else {
          throw std::runtime_error("Missing library name after " + arg);
        }
      } else if (arg == "--library-mirror" || arg == "-lm") {
        if (i + 1 < argc) {
          options.library_mirror_url = argv[++i];
        } else {
          throw std::runtime_error("Missing mirror URL after " + arg);
        }
      } else if (arg == "--list-libraries" || arg == "-ll") {
        options.list_libraries = true;
      } else if (arg == "--refresh-libraries" || arg == "-rl") {
        options.refresh_libraries = true;
      } else if (arg == "-c" || arg == "--compiler") {
        if (i + 1 < argc) {
          std::string compiler_name = argv[++i];
          auto it = Constants::COMPILER_TYPE_MAP.find(compiler_name);
          if (it != Constants::COMPILER_TYPE_MAP.end()) {
            options.compiler.type = it->second;
            options.compiler.name = compiler_name;
          }
        } else {
          throw std::runtime_error("Missing compiler name after " + arg);
        }
      } else if (arg == "-cp" || arg == "--compiler-path") {
        if (i + 1 < argc) {
          options.compiler.path = Utils::clean_path(argv[++i]);
        } else {
          throw std::runtime_error("Missing compiler path after " + arg);
        }
      } else if (arg == "-cppstd" || arg == "--cpp-standard") {
        if (i + 1 < argc) {
          options.compiler.cppStandard = argv[++i];
        } else {
          throw std::runtime_error("Missing C++ standard after " + arg);
        }
      } else if (arg == "-cstd" || arg == "--c-standard") {
        if (i + 1 < argc) {
          options.compiler.cStandard = argv[++i];
        } else {
          throw std::runtime_error("Missing C standard after " + arg);
        }
      } else if (arg == "-d" || arg == "--debugger") {
        if (i + 1 < argc) {
          std::string debugger_name = argv[++i];
          auto it = Constants::DEBUGGER_TYPE_MAP.find(debugger_name);
          if (it != Constants::DEBUGGER_TYPE_MAP.end()) {
            options.debugger.type = it->second;
            options.debugger.name = debugger_name;
          }
        } else {
          throw std::runtime_error("Missing debugger name after " + arg);
        }
      } else if (arg == "-dp" || arg == "--debugger-path") {
        if (i + 1 < argc) {
          options.debugger.path = Utils::clean_path(argv[++i]);
        } else {
          throw std::runtime_error("Missing debugger path after " + arg);
        }
      } else if (arg == "-ea" || arg == "--extra-args") {
        if (i + 1 < argc) {
          std::string extra_args = argv[++i];
          std::istringstream iss(extra_args);
          std::string flag;
          while (iss >> flag) {
            options.compiler.extraArgs.push_back(flag);
          }
        } else {
          throw std::runtime_error("Missing extra arguments after " + arg);
        }
      } else if (arg == "--help" || arg == "-h") {
        options.show_help = true;
      } else {
        throw std::runtime_error("Unknown option: " + arg);
      }
    }

    return options;
  }

  static void print_help(const std::string &program_name) {
    std::cout
        << "Usage: " << program_name << " [options]\n\n"
        << "Project Creation Options:\n"
        << "  -n, --name NAME           Set project name (default: "
        << Constants::DEFAULT_PROJECT_NAME << ")\n"
        << "  -p, --path PATH           Set project path (default: current "
           "directory)\n\n"
        << "Library Management Options:\n"
        << "  -I, --install LIB         Install third-party library\n"
        << "  -lm, --library-mirror URL Use custom library mirror\n"
        << "  -ll, --list-libraries     List available libraries\n"
        << "  -rl, --refresh-libraries  Refresh library information\n\n"
        << "Compiler Options:\n"
        << "  -c, --compiler COMPILER    Set compiler (gcc, g++, clang, "
           "clang++, cl)\n"
        << "  -cp, --compiler-path PATH   Set compiler path\n"
        << "  -cppstd, --cpp-standard STD Set C++ standard (c++98, c++11, "
           "etc.)\n"
        << "  -cstd, --c-standard STD     Set C standard (c99, c11, etc.)\n"
        << "  -ea, --extra-args ARGS      Set additional compiler flags\n\n"
        << "Debugger Options:\n"
        << "  -d, --debugger DEBUGGER    Set debugger (gdb, lldb)\n"
        << "  -dp, --debugger-path PATH   Set debugger path\n\n"
        << "Other Options:\n"
        << "  -v, --version             Output version information\n"
        << "  -h, --help                Show this help message\n\n"
        << "Examples:\n"
        << "  " << program_name << " -n myproject -I glfw -I sdl2\n"
        << "  " << program_name
        << " --library-mirror https://example.com/libs.json\n"
        << "  " << program_name << " --list-libraries\n";
  }

  static void print_version() {
    std::cout << "SLN2Code Version: " << Constants::VERSION << "\n"
              << "Maintainer: Macintosh-Maisensei\n"
              << "Repository: https://github.com/Macintosh-MaiSensei/SLN2Code\n"
              << "License: MIT\n";
  }
};

void print_logo() {
  std::cout << "  ____  _     _   _ ____   ____          _      " << "\n"
            << " / ___|| |   | \\ | |___ \\ / ___|___   __| | ___ " << "\n"
            << " \\___ \\| |   |  \\| | __) | |   / _ \\ / _` |/ _ \\ " << "\n"
            << "  ___) | |___| |\\  |/ __/| |__| (_) | (_| |  __/" << "\n"
            << " |____/|_____|_| \\_|_____|\\____\\___/ \\__,_|\\___|" << "\n";
}

static bool run_command_show(const std::string &cmd) {
  std::cout << "Running: " << cmd << "\n";
  int rc = std::system(cmd.c_str());
  if (rc != 0) {
    std::cerr << "Command failed with exit code " << rc << "\n";
    return false;
  }
  return true;
}

static bool install_vcpkg_in_dir(const fs::path &root) {
#ifdef _WIN32
  std::string clone_cmd =
      "git clone https://github.com/microsoft/vcpkg.git \"" + root.string() +
      "/vcpkg\"";
  if (!run_command_show(clone_cmd))
    return false;
  std::string bootstrap =
      "cd \"" + (root / "vcpkg").string() + "\" && \\" + "bootstrap-vcpkg.bat";
  return run_command_show(bootstrap);
#else
  std::string clone_cmd =
      "git clone https://github.com/microsoft/vcpkg.git \"" + root.string() +
      "/vcpkg\"";
  if (!run_command_show(clone_cmd))
    return false;
  std::string bootstrap =
      "cd \"" + (root / "vcpkg").string() + "\" && ./bootstrap-vcpkg.sh";
  return run_command_show(bootstrap);
#endif
}

static void ensure_vcpkg_available() {
  fs::path cwd = fs::current_path();
  fs::path vcpkg_dir = cwd / "vcpkg";
  if (fs::exists(vcpkg_dir)) {
    return;
  }
  std::cout << "vcpkg not found in current directory (" << cwd
            << ") . Install vcpkg? (y/N): ";
  std::string ans;
  std::getline(std::cin, ans);
  if (!ans.empty() && (ans[0] == 'y' || ans[0] == 'Y')) {
    std::cout << "Installing vcpkg into " << vcpkg_dir << " ...\n";
    if (!install_vcpkg_in_dir(cwd)) {
      std::cerr << "vcpkg installation failed.\n";
    } else {
      std::cout << "vcpkg installed successfully.\n";
    }
  } else {
    std::cout << "Skipping vcpkg installation.\n";
  }
}

int main(int argc, char *argv[]) {
  curl_global_init(CURL_GLOBAL_DEFAULT);

  try {

    CommandLineParser::Options options = CommandLineParser::parse(argc, argv);

    if (options.show_help) {
      CommandLineParser::print_help(argv[0]);
      return 0;
    }

    if (options.show_version) {
      print_logo();
      CommandLineParser::print_version();
      return 0;
    }

    if (options.list_libraries) {
      print_logo();
      LibraryService::list_available_libraries();
      return 0;
    }

    if (options.refresh_libraries) {
      print_logo();
      if (LibraryService::refresh_library_info()) {
        std::cout << "Library information refreshed successfully.\n";
      } else {
        std::cerr << "Failed to refresh library information.\n";
        return 1;
      }
      return 0;
    }

    if (!options.library_mirror_url.empty()) {
      if (!LibraryService::configure_library_mirror(
              options.library_mirror_url)) {
        std::cerr << "Failed to configure library mirror.\n";
        return 1;
      }
    }

    ensure_vcpkg_available();

    if (options.project_name == Constants::DEFAULT_PROJECT_NAME) {
      print_logo();
      CommandLineParser::print_version();
      std::cout << "\n";

      std::cout << "Enter project name (default: "
                << Constants::DEFAULT_PROJECT_NAME << "): ";
      std::string input_name;
      std::getline(std::cin, input_name);

      if (!input_name.empty()) {
        options.project_name = Utils::get_valid_name(input_name);
      }

      std::cout << "Enter project path (default: current directory): ";
      std::string input_path;
      std::getline(std::cin, input_path);

      if (!input_path.empty()) {
        options.base_path = Utils::clean_path(input_path);
      }
    }

    const fs::path project_full_path =
        fs::path(options.base_path) / options.project_name;

    if (fs::exists(project_full_path)) {
      std::cout << "Warning: Directory already exists: " << project_full_path
                << "\n";
      std::cout << "Do you want to continue? Existing files may be "
                   "overwritten. [y/N]: ";
      std::string response;
      std::getline(std::cin, response);

      if (response.empty() || (response[0] != 'y' && response[0] != 'Y')) {
        std::cout << "Project creation canceled.\n";
        return 0;
      }
    }

    std::cout << "\nCreating project \"" << options.project_name << "\" at:\n"
              << "  " << project_full_path << "\n\n";

    if (options.compiler.name.empty()) {
      options.compiler = CompilerService::get_compiler_config();
    } else {
      std::cout << "Compiler: " << options.compiler.name << " ("
                << options.compiler.path << ")\n";
    }

    if (options.debugger.name.empty()) {
      options.debugger = DebuggerService::get_debugger_config();
    } else {
      std::cout << "Debugger: " << options.debugger.name << " ("
                << options.debugger.path << ")\n";
    }

    std::cout << "C++ Standard: " << options.compiler.cppStandard << "\n";
    std::cout << "C Standard: " << options.compiler.cStandard << "\n\n";

    DirectoryNode project_structure =
        ProjectStructureService::get_project_structure(options.project_name);
    bool success = ProjectStructureService::create_directory_recursive(
        options.base_path, project_structure);

    if (!success) {
      std::cerr << "\nFailed to create project directories!\n";
      return 1;
    }

    if (!options.libraries_to_install.empty()) {
      for (const auto &lib_name : options.libraries_to_install) {
        LibraryService::add_third_party_library(project_full_path, lib_name);
      }
    } else {
      LibraryService::offer_library_installation(project_full_path);
    }

    ProjectGenerator::generate_project_files(
        project_full_path, options.project_name, options.compiler,
        options.debugger);

    std::cout << "\nProject \"" << options.project_name
              << "\" created successfully!\n";
    std::cout << "Configuration files generated in .vscode directory:\n";
    std::cout << "  - c_cpp_properties.json\n";
    std::cout << "  - tasks.json\n";
    std::cout << "  - launch.json\n";
    std::cout << "  - settings.json\n\n";

    if (!options.libraries_to_install.empty() ||
        fs::exists(project_full_path / "third_party")) {
      std::cout << "Third-party libraries installed in:\n";
      std::cout << "  - third_party/\n";
      std::cout << "  - docs/ (usage guides)\n\n";
    }

    std::cout << "Next steps:\n"
              << "  1. cd " << project_full_path << "\n"
              << "  2. code . (to open in VS Code)\n"
              << "  3. Review and update configuration files if needed\n"
              << "  4. Press F5 to build and debug!\n\n";

    std::cout << "Press Enter to exit...\n";
    std::cin.get();
    curl_global_cleanup();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    curl_global_cleanup();
    return 1;
  }
}

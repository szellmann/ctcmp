
#pragma once

#include <atomic>
#include <filesystem>
#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace util {

static std::string trim(std::string str, std::string ws = " \t")
{
  // Remove leading whitespace
  auto first = str.find_first_not_of(ws);

  // Only whitespace found
  if (first == std::string::npos)
    return "";

  // Remove trailing whitespace
  auto last = str.find_last_not_of(ws);

  // No whitespace found
  if (last == std::string::npos)
    last = str.size() - 1;

  // Skip if empty
  if (first > last)
    return "";

  // Trim
  return str.substr(first, last - first + 1);
}

static std::vector<std::string> string_split(std::string s, char delim)
{
  std::vector<std::string> result;

  std::istringstream stream(s);

  for (std::string token; std::getline(stream, token, delim); ) {
    result.push_back(token);
  }

  return result;
}

struct FileMonitor {
  FileMonitor(const std::string &fileName) : fileToMonitor(fileName)
  {}

  ~FileMonitor() {
    quit=true;
    thrd.join();
  }

  void run() {
    thrd = std::thread([this]() {
      for (;;) {
        auto t = std::filesystem::last_write_time(fileToMonitor);

        if (t > lastWriteTime) {
          handlerFunc(userData);
          lastWriteTime = t;
        }

        if (quit) {
          break;
        }
      }
    });
  }

  std::thread thrd;
  std::filesystem::path fileToMonitor;
  std::filesystem::file_time_type lastWriteTime;
  std::function<void(void *)> handlerFunc;
  void *userData{nullptr};
  std::atomic<bool> quit{false};
};

} // namespace util

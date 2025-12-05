#pragma once

#include <filesystem>
#include <iostream>
#include <sstream>
#include <memory>

#include <framework/status.h>

namespace fw {

// this is called automatically in Framework::initialize()
// TODO: make this a static member of the Logger class?
fw::Status LogInitialize();

// Writes the given string to the log file and possible console depending on flags. Generally you
// don't want to call this directly, but instead use the Logger helper class.
void LogWriteRaw(std::string_view msg);

// Returns the name of the log file (or empty string if we're not logging to a file).
std::filesystem::path LogFileName();

// Logger is a class that you create a temporary instance off, stream data to it, and when the
// instance is destructed, the final message is written to the log file (and, optionally, the
// console). Sample usage as in:
//
//  fw::Logger(__FILE__, __LINE__, fw::Logger::Level::kInfo) << "Hello world: " << 42 << etc.
//
// Or, use the handy preprocess macro wrapper:
//
//  LOG(INFO) << "Hello world: " << 42 << variable;
class Logger {
public:
  enum class Level {
    kDebug = 1,
    kInfo,
    kWarning,
    kError
  };

  Logger(std::string_view file, int line, Level level);
  ~Logger();

  template<typename T> std::ostream &operator <<(T const &t);

private:
  bool enabled_;
  std::stringstream buffer_;
};

template<typename T>
inline std::ostream &Logger::operator <<(T const &t) {
  return buffer_ << t;
}

#define DBG fw::Logger::Level::kDebug
#define INFO  fw::Logger::Level::kInfo
#define WARN  fw::Logger::Level::kWarning
#define ERR fw::Logger::Level::kError

#define LOG(level) fw::Logger(__FILE__, __LINE__, level)

}

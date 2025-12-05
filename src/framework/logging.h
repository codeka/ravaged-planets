#pragma once

#include <filesystem>
#include <iostream>
#include <sstream>
#include <memory>

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>

namespace fw {

#if defined(UNIX) || defined(__APPLE__)
#define THREADLOCAL __thread
#else
#define THREADLOCAL thread_local
#endif

// this is a "log sink" that we'll provide to the boost.iostreams
// library for actual writing...
class LogSink {
public:
  typedef char char_type;
  typedef boost::iostreams::sink_tag category;

  LogSink();
  LogSink(LogSink const &copy);
  ~LogSink();

  std::streamsize write(const char* s, std::streamsize n);
};

// this is a wrapper for the log object which keeps the actual stream
// in thread-local storage
class LogWrapper {
private:
  std::filesystem::path filename_;
  static THREADLOCAL std::ostream *log_;
  LogSink sink_;

public:
  LogWrapper();

  // Writes the given string directly to the log without prepending the time, etc.
  void WriteRaw(std::string_view msg);

  template<typename T> std::ostream &operator <<(T const &t);
};

//----------------------------------------------------------------------------
extern LogWrapper debug;

// this is called automatically in Framework::initialize()
// TODO: make this a static member of the Logger class?
void LoggingInitialize();

inline void LogWrapper::WriteRaw(std::string_view msg) {
  sink_.write(msg.data(), msg.length());
}

// This is the main implementation. Basically, we get the instance
// of the log ostream from thread-local storage and call it's operator <<
// to do the *actual* work...
template<typename T>
inline std::ostream &LogWrapper::operator <<(T const &t) {
  std::ostream *log = log_;
  if (log == nullptr) {
    boost::iostreams::stream_buffer<LogSink> *buffer = new boost::iostreams::stream_buffer<LogSink>(sink_);
    log = new std::ostream(buffer);
    log_ = log;
  }

  return (*log) << t;
}

//----------------------------------------------------------------------------

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

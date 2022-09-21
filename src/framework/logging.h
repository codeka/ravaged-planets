#pragma once

#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/shared_ptr.hpp>

namespace fw {

#if defined(UNIX) || defined(__APPLE__)
#define THREADLOCAL __thread
#else
#define THREADLOCAL thread_local
#endif

// this is a "log sink" that we'll provide to the boost.iostreams
// library for actual writing...
class LogSink {
private:
  bool open_;
  boost::filesystem::path filename_;
  boost::shared_ptr<std::ofstream> outs_;

public:
  typedef char char_type;
  typedef boost::iostreams::sink_tag category;

  LogSink();
  LogSink(LogSink const &copy);
  ~LogSink();

  void open(boost::filesystem::path const &filename);
  std::streamsize write(const char* s, std::streamsize n);
};

// this is a wrapper for the log object which keeps the actual stream
// in thread-local storage
class LogWrapper {
private:
  boost::filesystem::path filename_;
  static THREADLOCAL std::ostream *log_;
  LogSink sink_;

public:
  LogWrapper();

  void initialize(boost::filesystem::path const &filename);
  boost::filesystem::path get_filename() const { return filename_; }

  template<typename T> std::ostream &operator <<(T const &t);
};

//----------------------------------------------------------------------------
extern LogWrapper debug;

// this is called automatically in Framework::initialize()
void logging_initialize();

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

}

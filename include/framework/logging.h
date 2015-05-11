#pragma once

#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

namespace fw {

  // this is a "log sink" that we'll provide to the boost.iostreams
  // library for actual writing...
  class log_sink {
  private:
    bool _open;
    boost::filesystem::path _filename;
    boost::shared_ptr<std::ofstream> _outs;

  public:
    typedef char char_type;
    typedef boost::iostreams::sink_tag category;

    log_sink();
    log_sink(log_sink const &copy);
    ~log_sink();

    void open(boost::filesystem::path const &filename);
    std::streamsize write(const char* s, std::streamsize n);
  };

  // this is a wrapper for the log object which keeps the actual stream
  // in thread-local storage
  class log_wrapper {
  private:
    boost::filesystem::path _filename;
    boost::thread_specific_ptr<std::ostream> _log;
    log_sink _sink;

    // this is the cleanup function we'll use for when our thread exits
    static void cleanup(std::ostream *stream);

  public:
    log_wrapper();

    void initialize(boost::filesystem::path const &filename);
    boost::filesystem::path get_filename() const { return _filename; }

    template<typename T> std::ostream &operator <<(T const &t);
  };

  //----------------------------------------------------------------------------
  extern log_wrapper debug;

  // this is called automatically in framework::initialize()
  void logging_initialize();

  // This is the main implementation. Basically, we get the instance
  // of the log ostream from thread-local storage and call it's operator <<
  // to do the *actual* work...
  template<typename T>
  inline std::ostream &log_wrapper::operator <<(T const &t) {
    std::ostream *log = _log.get();
    if (log == nullptr) {
      boost::iostreams::stream_buffer<log_sink> *buffer = new boost::iostreams::stream_buffer<log_sink>(_sink);
      log = new std::ostream(buffer);
      _log.reset(log);
    }

    return (*log) << t;
  }
}
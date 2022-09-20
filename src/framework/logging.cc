#include <ctime>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <framework/logging.h>
#include <framework/settings.h>
#include <framework/paths.h>

namespace io = boost::iostreams;
namespace fs = boost::filesystem;

namespace fw {

  LogWrapper debug;
  static std::mutex mutex;
  static bool log_to_console = false;

  THREADLOCAL std::ostream *LogWrapper::log_;

  void logging_initialize() {
    settings stg;

    fs::path log_path;
    std::string logfilename = stg.get_value<std::string>("debug-logfile");
    if (logfilename != "") {
      log_path = resolve(logfilename, true);
    }

    if (stg.is_set("debug-console")) {
      log_to_console = true;
    }

    debug.initialize(log_path);
    debug << "Logging started." << std::endl;
    debug << "Install base: " << fw::install_base_path() << std::endl;
    debug << "User base: " << fw::user_base_path() << std::endl;
  }

  //-------------------------------------------------------------------------
  LogWrapper::LogWrapper() {
  }

  void LogWrapper::initialize(fs::path const &filename) {
    filename_ = filename;
    if (filename_ != fs::path()) {
      sink_.open(filename_);
    }
  }

  //-------------------------------------------------------------------------
  LogSink::LogSink()
    : open_(false), outs_(new std::ofstream()) {
  }

  LogSink::LogSink(LogSink const &copy)
    : open_(copy.open_), outs_(copy.outs_), filename_(copy.filename_) {
  }

  LogSink::~LogSink() {
  }

  void LogSink::open(fs::path const &filename) {
    filename_ = filename;
    outs_->open(filename_.string().c_str());
    open_ = true;
  }

  std::streamsize LogSink::write(const char *s, std::streamsize n) {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t c_now = std::chrono::system_clock::to_time_t(now);

    {
      std::unique_lock<std::mutex> lock(mutex);
      if (open_) {
        try {
          (*outs_) << std::put_time(std::localtime(&c_now), "%F %T") << " : ";
          outs_->write(s, n);
          outs_->flush();
        } catch (std::exception &e) {
          std::cerr << e.what();
        }
      }

      if (log_to_console) {
#if defined(_WIN32)
        std::stringstream ss;
        ss << std::put_time(std::localtime(&c_now), "%F %T") << " : ";
        ss.write(s, n);
        ::OutputDebugString(ss.str().c_str());
#endif

        std::cout << std::put_time(std::localtime(&c_now), "%F %T") << " : ";
        std::cout.write(s, n);
      }
    }

    return n;
  }
}

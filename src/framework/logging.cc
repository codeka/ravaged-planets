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

  log_wrapper debug;
  static std::mutex mutex;
  static bool log_to_console = false;

  THREADLOCAL std::ostream *log_wrapper::_log;

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
  log_wrapper::log_wrapper() {
  }

  void log_wrapper::initialize(fs::path const &filename) {
    _filename = filename;
    if (_filename != fs::path()) {
      _sink.open(_filename);
    }
  }

  //-------------------------------------------------------------------------
  log_sink::log_sink()
    : _open(false), _outs(new std::ofstream()) {
  }

  log_sink::log_sink(log_sink const &copy)
    : _open(copy._open), _outs(copy._outs), _filename(copy._filename) {
  }

  log_sink::~log_sink() {
  }

  void log_sink::open(fs::path const &filename) {
    _filename = filename;
    _outs->open(_filename.string().c_str());
    _open = true;
  }

  std::streamsize log_sink::write(const char *s, std::streamsize n) {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t c_now = std::chrono::system_clock::to_time_t(now);

    {
      std::unique_lock<std::mutex> lock(mutex);
      if (_open) {
        try {
          (*_outs) << std::put_time(std::localtime(&c_now), "%F %T") << " : ";
          _outs->write(s, n);
          _outs->flush();
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
#else
        std::cout << std::put_time(std::localtime(&c_now), "%F %T") << " : ";
        std::cout.write(s, n);
#endif
      }
    }

    return n;
  }
}

#include <iostream>
#include <fstream>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <framework/logging.h>
#include <framework/settings.h>
#include <framework/paths.h>

namespace io = boost::iostreams;
namespace fs = boost::filesystem;

namespace fw {

  log_wrapper debug;
  static boost::mutex mutex;
  static bool log_to_console = false;

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
  log_wrapper::log_wrapper()
    : _log(&log_wrapper::cleanup) {
  }

  void log_wrapper::initialize(fs::path const &filename) {
    _filename = filename;
    if (_filename != fs::path()) {
      _sink.open(_filename);
    }
  }

  void log_wrapper::cleanup(std::ostream *stream) {
    delete stream;
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
    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

    {
      boost::lock_guard<boost::mutex> lock(mutex);
      if (_open) {
        (*_outs) << now << " : ";
        _outs->write(s, n);
        _outs->flush();
      }

      if (log_to_console) {
        std::cout << now << " : ";
        std::cout.write(s, n);
      }
    }

    return n;
  }
}

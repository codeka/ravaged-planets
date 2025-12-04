#include <ctime>
#include <chrono>
#include <filesystem>
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
namespace fs = std::filesystem;

namespace fw {

LogWrapper debug;
static std::mutex mutex;

static bool g_log_to_console = false;
static fs::path g_log_filename;
static Logger::Level g_max_level = Logger::Level::kDebug;

// The file we're writing to, or nullptr if we're not writing to a file.
static std::unique_ptr<std::ofstream> g_outs;

THREADLOCAL std::ostream *LogWrapper::log_;

void LoggingInitialize() {
  std::string logfilename = Settings::get<std::string>("debug-logfile");
  if (logfilename != "") {
    g_log_filename = resolve(logfilename, true);
    g_outs = std::make_unique<std::ofstream>();
    g_outs->open(g_log_filename);
  }

  g_log_to_console = Settings::get<bool>("debug-console");

  std::string log_level = Settings::get<std::string>("log-level");
  if (log_level[0] == 'D' || log_level[0] == 'd') {
    g_max_level = Logger::Level::kDebug;
  } else if (log_level[0] == 'I' || log_level[0] == 'i') {
    g_max_level = Logger::Level::kInfo;
  } else if (log_level[0] == 'W' || log_level[0] == 'w') {
    g_max_level = Logger::Level::kWarning;
  } else if (log_level[0] == 'E' || log_level[0] == 'e') {
    g_max_level = Logger::Level::kError;
  }

  LOG(INFO) << "Logging started.";
  LOG(INFO) << "Install base: " << fw::install_base_path().string();
  LOG(INFO) << "User base: " << fw::user_base_path().string();
}

//-------------------------------------------------------------------------
LogWrapper::LogWrapper() {
}

//-------------------------------------------------------------------------
LogSink::LogSink() {}

LogSink::LogSink(LogSink const &copy) {}

LogSink::~LogSink() {
}

std::streamsize LogSink::write(const char *s, std::streamsize n) {
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  std::time_t c_now = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&c_now), "%F %T") << " : ";
  ss.write(s, n);
  LogWriteRaw(ss.str());

  return n;
}

//-------------------------------------------------------------------------

fs::path LogFileName() {
  return g_log_filename;
}

void LogWriteRaw(std::string_view msg) {
    if (g_outs != nullptr) {
      g_outs->write(msg.data(), msg.length());
      g_outs->flush();
    }

    if (g_log_to_console) {
#if defined(_WIN32)
      ::OutputDebugString(msg.data());
#endif

      std::cout.write(msg.data(), msg.length());
    }
}

Logger::Logger(std::string_view file, int line, Logger::Level level) {
  enabled_ = level >= g_max_level;
  if (!enabled_) {
    return;
  }

  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  std::time_t c_now = std::chrono::system_clock::to_time_t(now);
  buffer_ << std::put_time(std::localtime(&c_now), "%F %T") << " : ";

  fs::path file_path(file);
  buffer_ << "[" << file_path.filename().string() << ":" << line << "] ";

  switch (level) {
  case Level::kDebug:
    buffer_ << "DEBUG ";
    break;
  case Level::kInfo:
    buffer_ << "INFO ";
    break;
  case Level::kWarning:
    buffer_ << "WARN ";
    break;
  case Level::kError:
    buffer_ << "ERROR ";
    break;
  default:
    buffer_ << "UNKNOWN ";
    break;
  }
}

Logger::~Logger() {
  if (!enabled_) {
    return;
  }

  buffer_ << std::endl;
  fw::LogWriteRaw(buffer_.str());
}

}  // namespace fw


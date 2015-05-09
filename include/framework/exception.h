#pragma once

#include <string>
#include <exception>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>

namespace fw {

// this can be added to an exception after a system call that sets errno
typedef boost::error_info<struct tag_errno, int> errno_error_info;

// this can be added to an exception if you know the "filename" we were accessing at the time of the exception
typedef boost::error_info<struct tag_filename, boost::filesystem::path> filename_error_info;

// this can be added to an exception to provide an error message as to what you were
// trying to do. this message doesn't always have to be useful to the end-user, but
// sometimes it might be.
typedef boost::error_info<struct tag_message, std::string> message_error_info;

// this can be added when we want to include the value of SDL_GetError() in the exception.
typedef boost::error_info<struct tag_sdl_error, char const *> sdl_error_info;

// this is automatically added to fw::exception and includes the stack trace at the point
// where the exception is thrown...
typedef boost::error_info<struct tag_stacktrace, std::vector<std::string> > stacktrace_error_info;

// this is automatically added to fw::exception and include information about all loaded modules
// at the time the exception occurred.
typedef boost::error_info<struct tag_loadedmodules, std::vector<std::string> > loaded_modules_error_info;

// this is an exception class that inherits from boost::exception and
// provides some extra features for throwing errors from windows (and direct x)
class exception: public boost::exception, public std::exception {
private:
  void populate_stacktrace();

public:
  // constructs a new win32_exception which will add the return value from
  // GetLastError to the exception
  inline exception() {
    populate_stacktrace();
  }

  inline virtual ~exception() throw () {
  }

  // this is just what we need to return for std::exception::what, it's just a
  // generic method, so we can't be too specific
  virtual char const *what() const throw ();

  // Generate a stack trace from the current call location.
  static std::vector<std::string> generate_stack_trace();

  // Helper method that logs (to fw::debug) the current stack trace (useful only for debugging).
  static void log_stacktrace();
};

std::string to_string(errno_error_info const &err_info);
std::string to_string(stacktrace_error_info const &err_info);
std::string to_string(loaded_modules_error_info const &err_info);
//std::string to_string(sdl_error_info const &err_info);

}

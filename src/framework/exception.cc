#include <boost/foreach.hpp>
#include <boost/format.hpp>

#include <framework/logging.h>
#include <framework/exception.h>

namespace fw {

char const *Exception::what() const throw () {
  std::string const *msg = boost::get_error_info<message_error_info>(*this);
  if (msg != 0)
    return msg->c_str();
  return "Unhandled exception!";
}

/*
 * Adds a stacktrace_error_info to this exception, which contains a list of all the frames in the current stack.
 */
void Exception::populate_stacktrace() {
  (*this) << stacktrace_error_info(generate_stack_trace());
}

void Exception::log_stacktrace() {
  int n = 0;
  BOOST_FOREACH(std::string frame, generate_stack_trace()) {
    if (n++ < 2) {
      // Ignore the first two frame, they'll be this function and generate_stack_trace itself.
      continue;
    }
    fw::debug << "  " << frame << std::endl;
  }
}

std::string to_string(errno_error_info const &err_info) {
  int err = err_info.value();
  return (boost::format("(errno %1% \"%2%\") ") % err % strerror(err)).str();
}

std::string to_string(stacktrace_error_info const &err_info) {
  std::stringstream ss;
  ss << std::endl;
  BOOST_FOREACH(std::string line, err_info.value()) {
    ss << "  " << line << std::endl;
  }

  return ss.str();
}

std::string to_string(loaded_modules_error_info const &err_info) {
  std::stringstream ss;
  ss << std::endl;
  BOOST_FOREACH(std::string line, err_info.value()) {
    ss << line << std::endl;
  }

  return ss.str();
}

}

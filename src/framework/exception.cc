#include <boost/foreach.hpp>
#include <boost/format.hpp>

#include <framework/exception.h>

namespace fw {

char const *exception::what() const throw () {
  std::string const *msg = boost::get_error_info<message_error_info>(*this);
  if (msg != 0)
    return msg->c_str();
  return "Unhandled exception!";
}

std::string to_string(errno_error_info const &err_info) {
  int err = err_info.value();
  return (boost::format("(errno %1% \"%2%\") ") % err % strerror(err)).str();
}

std::string to_string(stacktrace_error_info const &err_info) {
  std::stringstream ss;
  ss << std::endl;
  BOOST_FOREACH(std::string line, err_info.value()) {
    ss << line << std::endl;
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

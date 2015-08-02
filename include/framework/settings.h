#pragma once

#include <string>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

namespace fw {

/**
 * This is the main "settings" class, which you can use to access global settings from the user's default.conf file.
 */
class settings
{
public:
  settings();
  ~settings();

  /**
   * You must call this at program startup (*before* you call the framework::initialize() method!) it'll parse
   * the command-line options, read the .conf file and so on.
   */
  static void initialize(boost::program_options::options_description const &additional_options,
      int argc, char **argv, std::string const &options_file = "default.conf");

  /** Gets the map of all settings. */
  boost::program_options::variables_map &get_variables() const;

  /** Gets the value (as a \c boost::program_options::variable_value) of a single variable. */
  boost::program_options::variable_value const &get_variable_value(std::string const &name) const;

  /** Gets the value of the given setting, as the specified type. */
  template<typename T>
  inline T get_value(std::string const &name) {
    // this may throw a bad_any_cast exception if the variable is not of the given type
    return get_variable_value(name).as<T>();
  }

  /** Get a value which indicates whether the given boolean option is specified. */
  inline bool is_set(std::string const &name) {
    return (get_variables().count(name) > 0);
  }

  /** Prints out the help message associated with our options. */
  void print_help();

  /** Gets the full path to the executable. */
  boost::filesystem::path get_executable_path() const;
};

}

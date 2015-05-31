#pragma once

#include <string>
#include <boost/program_options.hpp>

namespace fw {

  // This is the main "settings" class, which you can use to access global settings from the user's settings.ini file.
  //
  // Basic usage is as follows:
  //
  // fw::settings settings;
  // std::string s = settings.get_value<std::string>("name");
  // int i = settings.get_value<int>("name");
  // etc...
  class settings
  {
  public:
    settings();
    ~settings();

    // you must call this at program startup (*before* you call the framework::initialize()
    // method!) it'll parse the command-line options and so on.
    static void initialize(boost::program_options::options_description const &additional_options,
        int argc, char **argv, std::string const &options_file = "default.conf");

    boost::program_options::variables_map &get_variables() const;
    boost::program_options::variable_value const &get_variable_value(std::string const &name) const;

    // gets the value of the given setting, as the specified type
    template<typename T>
    inline T get_value(std::string const &name) {
      // this may throw a bad_any_cast exception if the variable is not of the given type
      return get_variable_value(name).as<T>();
    }

    // get a value which indicates whether the given boolean option is specified
    inline bool is_set(std::string const &name) {
      return (get_variables().count(name) > 0);
    }

    void print_help();
  };
}

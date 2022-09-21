
#include <boost/program_options.hpp>

#include "framework/settings.h"

namespace po = boost::program_options;

namespace game {

  void settings_initialize(int argc, char** argv) {
    po::options_description additional_options("Additional options");
    additional_options.add_options()
        ("server-url", po::value<std::string>()->default_value("http://svc.warworlds.codeka.com/"), "The URL we use to log in, find other games, and so on. Usually you won't change the default.")
        ("listen-port", po::value<std::string>()->default_value("9347"), "The port we listen on. You can specify a range with the syntax aaa-bbb")
        ("auto-login", po::value<std::string>()->default_value(""), "A string used to automatically log on to the server. The value is obfuscated.")
      ;

    po::options_description keybinding_options("Key bindings");
    keybinding_options.add_options()
        ("bind.pause", po::value<std::string>()->default_value("ESC"))
        ("bind.chat", po::value<std::string>()->default_value("TAB"))
        ("bind.select", po::value<std::string>()->default_value("Left-Mouse"))
        ("bind.deselect", po::value<std::string>()->default_value("Right-Mouse"))
        ("bind.screenshot", po::value<std::string>()->default_value("Ctrl+S"))
      ;

    po::options_description options;
    options.add(additional_options).add(keybinding_options);
    fw::Settings::initialize(options, argc, argv, "default.conf");
  }
}

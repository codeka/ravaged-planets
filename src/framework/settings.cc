#include <fstream>

#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <framework/exception.h>
#include <framework/settings.h>
#include <framework/paths.h>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace fw {

  static bool is_initialized = false;
  static po::variables_map g_variables_map;
  static po::options_description g_option_descriptions;

  settings::settings() {
    if (!is_initialized) {
      BOOST_THROW_EXCEPTION(fw::exception() << message_error_info("settings::initialize() has not been called."));
    }
  }

  settings::~settings() {
  }

  po::variables_map& settings::get_variables() const {
    return g_variables_map;
  }

  po::variable_value const &settings::get_variable_value(std::string const &name) const {
    po::variable_value const &val = g_variables_map[name];
    if (val.empty()) {
      BOOST_THROW_EXCEPTION(fw::exception()
          << fw::message_error_info("specified option does not exist, or has no value: " + name));
    }

    return val;
  }

  void settings::print_help() {
    std::cerr << g_option_descriptions << std::endl;
  }

  // you must call this at program startup (*before* you call the framework::initialize() method!) it'll parse the
  // command-line options and so on.
  void settings::initialize(po::options_description const &additional_options, int argc, char **argv,
      std::string const &options_file/* = "default.conf"*/) {
    po::options_description graphics_options("Graphics options");
    graphics_options.add_options()
        ("windowed-width", po::value<int>()->default_value(800), "The width of the window when running in windowed mode")
        ("windowed-height", po::value<int>()->default_value(600), "The height of the window when running in windowed mode")
        ("fullscreen-width,W", po::value<int>()->default_value(0), "The width of the screen when running in fullscreen mode")
        ("fullscreen-height,H", po::value<int>()->default_value(0), "The height of the screen when running in fullscreen mode")
        ("windowed", "Run in windowed mode")
        ("disable-antialiasing", "If specified, we'll disable fullscreen anti-aliasing (better performance, lower quality)")
      ;

    po::options_description audio_options("Audio options");
    audio_options.add_options()
        ("disable-audio", "If specified, audio is completely disabled (usually only useful for debugging)")
      ;

    po::options_description debugging_options("Debugging options");
    debugging_options.add_options()
        ("debug-logfile", po::value<std::string>()->default_value(""), "Name of the file to do debug logging to. If not specified, does not log.")
        ("debug-console", po::value<bool>()->default_value(true), "If set, we'll log to the console as well as the log file.")
        ("debug-libcurl", po::value<bool>()->default_value(false), "If true, debug HTTP requests and responses.")
      ;

    po::options_description other_options("Other options");
    other_options.add_options()
        ("help", "Prints this help message.")
        ("lang", po::value<std::string>()->default_value("en"), "Name of the language we'll use for display and UI, etc. Default is 'en' (English)")
      ;

    po::options_description keybinding_options("Keybindings");
    keybinding_options.add_options()
        ("bind.cam-left", po::value<std::string>()->default_value("Left"))
        ("bind.cam-right", po::value<std::string>()->default_value("Right"))
        ("bind.cam-forward", po::value<std::string>()->default_value("Up"))
        ("bind.cam-backward", po::value<std::string>()->default_value("Down"))
        ("bind.cam-rot-left", po::value<std::string>()->default_value("["))
        ("bind.cam-rot-right", po::value<std::string>()->default_value("]"))
        ("bind.cam-zoom-in", po::value<std::string>()->default_value("Plus"))
        ("bind.cam-zoom-out", po::value<std::string>()->default_value("Minus"))
        ("bind.cam-rot-mouse", po::value<std::string>()->default_value("Middle-Mouse"))
      ;

    g_option_descriptions.add(graphics_options)
        .add(audio_options)
        .add(debugging_options)
        .add(other_options)
        .add(additional_options)
        .add(keybinding_options);

    po::store(po::parse_command_line(argc, argv, g_option_descriptions), g_variables_map);

    std::ifstream default_ini((fw::install_base_path() / options_file).string().c_str());
    po::store(po::parse_config_file(default_ini, g_option_descriptions), g_variables_map);
    default_ini.close();

    std::ifstream custom_ini((fw::user_base_path() / options_file).string().c_str());
    po::store(po::parse_config_file(custom_ini, g_option_descriptions), g_variables_map);
    custom_ini.close();

    po::notify(g_variables_map);
    is_initialized = true;
  }
}

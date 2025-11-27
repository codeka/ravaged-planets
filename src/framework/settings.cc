#include <framework/settings.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/str_split.h>

#include <framework/exception.h>
#include <framework/logging.h>
#include <framework/misc.h>
#include <framework/paths.h>

namespace fs = std::filesystem;

namespace fw {
namespace {

constexpr bool DBG = true;

static std::unordered_map<std::string, SettingValue> g_variables_map;
static std::string g_option_descriptions;
static fs::path g_executable_path;

inline void dbg(std::string_view msg) {
  if (!DBG) {
    return;
  }

  std::cerr << msg << std::endl;
}

absl::StatusOr<Setting> FindSetting(SettingDefinition const &settings, std::string_view name) {
  for (const SettingGroup &group : settings.groups()) {
    for (const Setting &setting : group.settings) {
      if (setting.name == name) {
        return setting;
      }
    }
  }

  return absl::NotFoundError(absl::StrCat("unknown setting: ", name));
}

absl::StatusOr<SettingValue> ParseSettingValue(
    SettingDefinition const &settings,
    std::string_view name,
    std::string_view value) {
  const auto setting = FindSetting(settings, name);
  if (!setting.ok()) {
    return setting.status();
  }

  if (setting->type == SettingType::kString) {
    return SettingValue::of(std::string(value));
  } else if (setting->type == SettingType::kBool) {
    bool bool_value;
    if (value == "true" || value == "1") {
      bool_value = true;
    } else if (value == "false" || value == "0") {
      bool_value = false;
    } else {
      return absl::InternalError(absl::StrCat(name, " value must be boolean: ", value));
    }
    return SettingValue::of(bool_value);
  } else if (setting->type == SettingType::kInt) {
    int int_value;
    if (!absl::SimpleAtoi(value, &int_value)) {
      return absl::InternalError(absl::StrCat(name, " value must be integer: ", value));
    }
    return SettingValue::of(int_value);
  } else if (setting->type == SettingType::kFloat) {
    float float_value;
    if (!absl::SimpleAtof(value, &float_value)) {
      return absl::InternalError(absl::StrCat(name, " value must be float: ", value));
    }
    return SettingValue::of(float_value);
  }
  return absl::OkStatus();
}

absl::Status ParseConfigFile(fs::path const &path, SettingDefinition const &settings) {
  dbg(absl::StrCat("Parsing config file: ", path.string()));

  std::ifstream ins(path);
  if (!ins.is_open()) {
    // File doesn't exist, that's fine.
    return absl::OkStatus();
  }

  std::string line_str;
  while (std::getline(ins, line_str)) {
    std::pair<std::string_view, std::string_view> split = absl::StrSplit(line_str, '#'); // Strip comments
    std::string_view line = fw::StripSpaces(split.first); // Strip whitespace
    if (line.empty()) {
      continue;
    }

    split = absl::StrSplit(line, '=');
    std::string_view name = fw::StripSpaces(split.first);
    std::string_view value = fw::StripSpaces(split.second);
    dbg(absl::StrCat("  parsed: ", name, " = ", value));

    auto setting_value = ParseSettingValue(settings, name, value);
    if (!setting_value.ok()) {
      auto status = setting_value.status();
      status.Update(absl::InternalError(absl::StrCat("in config file: ", path.string())));
      return status;
    }

    // Only overwrite if not already set. For example, command-line options should override config
    // file settings.
    if (g_variables_map.find(std::string(name)) == g_variables_map.end()) {
      g_variables_map.emplace(std::string(name), *setting_value);
    }
  }

  return absl::OkStatus();
}

// Parse the command-line. We only have some very simple parsing logic here. All options must be of
// the form --name=value. We also support --name only for boolean values (which will be set to
// true). And "--name value" is also supported for convenience.
absl::Status ParseCommandLine(int argc, char **argv, SettingDefinition const &settings) {
  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (!arg.starts_with("--")) {
      return absl::InvalidArgumentError(absl::StrCat("invalid command-line argument: ", arg));
    }
    arg.remove_prefix(2);

    std::string_view name;
    std::string_view value;

    size_t equal_pos = arg.find('=');
    if (equal_pos != std::string_view::npos) {
      name = arg.substr(0, equal_pos);
      value = arg.substr(equal_pos + 1);
    } else {
      name = arg;
      // Check if the next argument is a value (doesn't start with --)
      if (i + 1 < argc) {
        std::string_view next_arg = argv[i + 1];
        if (!next_arg.starts_with("--")) {
          value = next_arg;
          ++i; // Consume the next argument
        }
      }
    }

    const auto setting = FindSetting(settings, name);
    if (!setting.ok()) {
      return setting.status();
    }

    // If no value is provided, and it's a boolean setting, set it to true.
    if (value.empty()) {
      if (setting->type == SettingType::kBool) {
        value = "true";
      } else {
        return absl::InvalidArgumentError(
            absl::StrCat("no value provided for setting: ", name));
      }
    }

    auto setting_value = ParseSettingValue(settings, name, value);
    if (!setting_value.ok()) {
      return setting_value.status();
    }

    g_variables_map.emplace(std::string(name), *setting_value);
  }

  return absl::OkStatus();
}

}  // anonymous namespace

/* static */
std::optional<SettingValue> Settings::get(std::string_view name) {
  const auto it = g_variables_map.find(std::string(name));
  if (it == g_variables_map.end()) {
    return std::nullopt;
  }

  return it->second;
}

/* static */
void Settings::print_help() {
  std::cerr << "Available options:" << std::endl;
  std::cerr << g_option_descriptions << std::endl;
}

/* static */
fs::path Settings::get_executable_path() {
  return g_executable_path;
}

// you must call this at program startup (*before* you call the Framework::initialize() method!)
// it'll parse the command-line options and so on.
absl::Status Settings::initialize(
    SettingDefinition const &additional_settings, int argc, char **argv,
    std::string_view options_file/* = "default.conf"*/) {
  g_executable_path = argv[0];

  SettingDefinition all_settings;
  all_settings.add_group("Graphics", "Graphics-related settings")
      .add_setting<bool>("windowed", "Run in windowed mode", true)
      .add_setting<int>(
          "windowed-width", "The width of the window when running in windowed mode", 1280)
      .add_setting<int>(
          "windowed-height", "The height of the window when running in windowed mode", 720)
      .add_setting<int>(
          "fullscreen-width", "The width of the screen when running in fullscreen mode", 0)
      .add_setting<int>(
          "fullscreen-height", "The height of the screen when running in fullscreen mode", 0)
      .add_setting<bool>(
          "disable-antialiasing",
          "If specified, we'll disable fullscreen anti-aliasing (better performance, "
          "lower quality)", false);

  all_settings.add_group("Audio", "Audio-related settings")
      .add_setting<bool>(
          "disable-audio",
          "If specified, audio is completely disabled (usually only useful for debugging)",
          false);

  all_settings.add_group("Debugging", "Debugging-related settings")
      .add_setting<std::string>(
          "debug-logfile",
          "Name of the file to do debug logging to. If not specified, does not log.",
          "")
      .add_setting<bool>(
          "debug-console", "If set, we'll log to the console as well as the log file.", true)
      .add_setting<bool>(
          "debug-libcurl", "If true, debug HTTP requests and responses.", false)
      .add_setting<bool>(
          "debug-view",
          "If true, show some debug info in the bottom-right of the screen.",
          false)
      .add_setting<std::string>(
          "dbghelp-path",
          "Windows-only, path to dbghelp.dll file.", "");

  all_settings.add_group("Other", "Other settings")
      .add_setting<bool>(
          "help", "If specified, we'll print out this help message and exit.", false)
      .add_setting<std::string>("data-path", "Path to load data files from.", "")
      .add_setting<std::string>(
          "lang",
          "Name of the language we'll use for display and UI, etc.", "en");

  all_settings.add_group("Keybindings", "Keybinding settings")
      .add_setting<std::string>(
          "bind.toggle-fullscreen", "Keybinding to toggle fullscreen mode", "Alt+Enter")
      .add_setting<std::string>("bind.cam-left", "Keybinding to move the camera left", "Left")
      .add_setting<std::string>("bind.cam-right", "Keybinding to move the camera right", "Right")
      .add_setting<std::string>("bind.cam-forward", "Keybinding to move the camera forward", "Up")
      .add_setting<std::string>(
          "bind.cam-backward", "Keybinding to move the camera backward", "Down")
      .add_setting<std::string>("bind.cam-rot-left", "Keybinding to rotate the camera left", "[")
      .add_setting<std::string>("bind.cam-rot-right", "Keybinding to rotate the camera right", "]")
      .add_setting<std::string>("bind.cam-zoom-in", "Keybinding to zoom the camera in", "Plus")
      .add_setting<std::string>("bind.cam-zoom-out", "Keybinding to zoom the camera out", "Minus")
      .add_setting<std::string>(
          "bind.cam-rot-mouse", "Keybinding to rotate the camera with the mouse", "Middle-Mouse");

  all_settings.merge(additional_settings);

  // Note: we need to parse the command-line first, because it could specify things like an
  // alternative data-path or config file.
  auto status = ParseCommandLine(argc, argv, all_settings);
  if (!status.ok()) {
    return status;
  }

  // Next if you have a user-specific config file, parse that. It will only set values that are not
  // set on the command-line.
  status = ParseConfigFile(fw::user_base_path() / options_file, all_settings);
  if (!status.ok()) {
    return status;
  }

  // Finally parse the system-wide config file. It will only set values that are not set by the
  // command-line or user-specific config file.
  status = ParseConfigFile(fw::install_base_path() / options_file, all_settings);
  if (!status.ok()) {
    return status;
  }

  // If all else fails, set the defaults based on the setting definitions. We can also take this
  // opportunity to build up the help message.
  for (const SettingGroup &group : all_settings.groups()) {
    g_option_descriptions += absl::StrCat("\n", group.name, ":\n");
    if (group.description != "") {
      g_option_descriptions += absl::StrCat("  ", group.description, "\n");
    }

    for (const Setting &setting : group.settings) {
      g_option_descriptions += absl::StrCat("  --", setting.name);
      if (setting.default_value.has_value()) {
        g_option_descriptions +=
            absl::StrCat(" [default: ", setting.default_value->value_str(), "]");
      }
      g_option_descriptions += "\n    ";
      g_option_descriptions += setting.description;
      g_option_descriptions += "\n";

      // if the setting isn't already set, and it has a default value, set it now.
      if (g_variables_map.find(setting.name) == g_variables_map.end() &&
          setting.default_value.has_value()) {
        g_variables_map.emplace(setting.name, *setting.default_value);
      }
    }

    g_option_descriptions += "\n";
  }

  return absl::OkStatus();
}

}

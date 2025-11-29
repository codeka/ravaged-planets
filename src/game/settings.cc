#include <game/settings.h>

#include <framework/settings.h>
#include <framework/status.h>

namespace game {

fw::Status settings_initialize(int argc, char** argv) {
  fw::SettingDefinition extra_settings;

  extra_settings.add_group("Game", "Game-specific settings")
      .add_setting<std::string>(
          "server-url",
          "The URL we use to log in, find other games, and so on. Usually you won't change the default.",
          "http://svc.warworlds.codeka.com/")
      .add_setting<std::string>(
          "listen-port",
          "The port we listen on. You can specify a range with the syntax aaa-bbb.",
          "9347")
      .add_setting<std::string>(
          "auto-login",
          "A string used to automatically log on to the server. The value is obfuscated.",
          "");

  extra_settings.add_group("Keybindings", "Keybinding settings")
      .add_setting<std::string>(
          "bind.pause", "Open the pause menu", "ESC")
      .add_setting<std::string>(
          "bind.chat", "Open the chat window", "TAB")
      .add_setting<std::string>(
          "bind.select", "Select the object under the mouse", "Left-Mouse")
      .add_setting<std::string>(
          "bind.deselect", "Deselect all objects", "Right-Mouse")
      .add_setting<std::string>(
          "bind.screenshot", "Take a screenshot", "Ctrl+S");

  return fw::Settings::initialize(extra_settings, argc, argv, "default.conf");
}

}

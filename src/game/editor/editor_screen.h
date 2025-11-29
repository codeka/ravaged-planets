#pragma once

#include <framework/status.h>

#include <game/screens/screen.h>

namespace ed {
class Tool;
class EditorTerrain;
class EditorWorld;

class EditorScreen: public game::Screen {
private:
  Tool *tool_;
  EditorWorld *world_;

public:
  EditorScreen();
  virtual ~EditorScreen();

  virtual void show();
  virtual void hide();

  // updates the "world", this is only called if we're the active Screen
  virtual void update();

  // create a new map to edit
  void new_map(int width, int height);

  // load an existing map to edit
  fw::Status open_map(std::string const &name);

  Tool *get_active_tool() {
    return tool_;
  }
  void set_active_tool(std::string const &name);

  // This is just a helper that returns the current editor_screen instance
  static EditorScreen *get_instance();
};

}

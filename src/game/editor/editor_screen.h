#pragma once

#include <game/screens/screen.h>

namespace ed {
class tool;
class editor_terrain;
class editor_world;

class editor_screen: public game::screen {
private:
  tool *_tool;
  editor_world *_world;

public:
  editor_screen();
  virtual ~editor_screen();

  virtual void show();
  virtual void hide();

  // updates the "world", this is only called if we're the active screen
  virtual void update();

  // renders this screen, this is only called if we're the active screen
  virtual void render(fw::sg::Scenegraph &Scenegraph);

  // create a new map to edit
  void new_map(int width, int height);

  // load an existing map to edit
  void open_map(std::string const &name);

  tool *get_active_tool() {
    return _tool;
  }
  void set_active_tool(std::string const &name);

  // This is just a helper that returns the current editor_screen instance
  static editor_screen *get_instance();
};

}

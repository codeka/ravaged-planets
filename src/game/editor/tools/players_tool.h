#pragma once

#include <game/editor/tools/tools.h>

namespace fw {
class Model;
}

class players_tool_window;

namespace ed {
class world;

class players_tool: public tool {
private:
  players_tool_window *wnd_;
  std::shared_ptr<fw::Model> _marker;
  int player_no_;

  void on_key(std::string keyname, bool is_down);

public:
  players_tool(editor_world *wrld);
  virtual ~players_tool();

  virtual void activate();
  virtual void deactivate();

  virtual void render(fw::sg::Scenegraph &Scenegraph);

  // sets the current player_number we're editing the starting location for
  void set_curr_player(int player_no);
  int get_curr_player() const {
    return player_no_;
  }
};

}

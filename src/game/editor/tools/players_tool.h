#pragma once

#include <string_view>

#include <game/editor/tools/tools.h>

namespace fw {
class Model;
}

class PlayersToolWindow;

namespace ed {

class PlayersTool: public Tool {
private:
  std::unique_ptr<PlayersToolWindow> wnd_;
  std::shared_ptr<fw::Model> marker_;
  int player_no_;

  void on_key(std::string_view keyname, bool is_down);

public:
  PlayersTool(EditorWorld *wrld);
  virtual ~PlayersTool();

  virtual void activate();
  virtual void deactivate();

  // sets the current player_number we're editing the starting location for
  void set_curr_player(int player_no);
  int get_curr_player() const {
    return player_no_;
  }
};

}

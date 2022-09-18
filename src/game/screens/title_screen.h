#pragma once

#include <game/screens/screen.h>

namespace fw {
namespace gui {
class widget;
}
}

namespace game {
class main_menu_window;
class new_game_window;
class new_ai_player_window;

/**
 * The title screen is the first screen we load up in the game. It contains the UI for starting a new game, loading a
 * save game, joining a multiplayer game and so on.
 */
class title_screen: public screen {
private:
  main_menu_window *_main_menu_window;
  new_game_window *_new_game_window;
  new_ai_player_window *_new_ai_player_window;

public:
  title_screen();
  virtual ~title_screen();

  virtual void show();
  virtual void hide();

  virtual void update();
};

}

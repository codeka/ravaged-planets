#pragma once

#include <game/screens/screen.h>

namespace game {
class MainMenuWindow;
class NewGameWindow;
class NewAIPlayerWindow;

// The title screen is the first screen we load up in the game. It contains the UI for starting a new game, loading a
// save game, joining a multiplayer game and so on.
class TitleScreen: public Screen {
private:
  MainMenuWindow *main_menu_window_;
  NewGameWindow *new_game_window_;
  NewAIPlayerWindow *new_ai_player_window_;

public:
  TitleScreen();
  virtual ~TitleScreen();

  virtual void show();
  virtual void hide();

  virtual void update();
};

}

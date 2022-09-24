#include <framework/framework.h>
#include <framework/misc.h>
#include <framework/logging.h>

#include <game/application.h>
#include <game/screens/screen.h>
#include <game/screens/game_screen.h>
#include <game/screens/title_screen.h>
#include <game/screens/title/main_menu_window.h>
#include <game/screens/title/new_ai_player_window.h>
#include <game/screens/title/new_game_window.h>

namespace game {

TitleScreen::TitleScreen() : main_menu_window_(nullptr), new_game_window_(nullptr) {
}

TitleScreen::~TitleScreen() {
}

void TitleScreen::show() {
  main_menu_window_ = new MainMenuWindow();
  new_ai_player_window_ = new NewAIPlayerWindow();
  new_game_window_ = new NewGameWindow();
  main_menu_window_->initialize(new_game_window_);
  new_game_window_->initialize(main_menu_window_, new_ai_player_window_);
  new_ai_player_window_->initialize(new_game_window_);

  main_menu_window_->show();
}

void TitleScreen::hide() {
  delete main_menu_window_;
  main_menu_window_ = nullptr;
  delete new_ai_player_window_;
  new_ai_player_window_ = nullptr;
  delete new_game_window_;
  new_game_window_ = nullptr;
}

void TitleScreen::update() {
  main_menu_window_->update();
  new_game_window_->update();
}

}

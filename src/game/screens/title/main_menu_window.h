#pragma once

namespace fw {
namespace gui {
class window;
class widget;
}
}

namespace game {
class new_game_window;

/**
 * This is the main menu, it's got such things as the "New Game", "Join Game", etc buttons.
 */
class main_menu_window {
private:
  new_game_window *_new_game_window;
  fw::gui::window *_wnd;
  bool _exiting;

  bool new_game_clicked(fw::gui::widget *w);
  bool join_game_clicked(fw::gui::widget *w);
  bool quit_clicked(fw::gui::widget *w);
  bool editor_clicked(fw::gui::widget *w);

public:
  main_menu_window();
  ~main_menu_window();
  void initialize(new_game_window *new_game_window);

  void update();
  void show();
  void hide();
};

}

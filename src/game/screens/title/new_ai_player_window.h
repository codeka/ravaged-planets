#pragma once

namespace fw{
namespace gui {
class Window;
class Widget;
} }

namespace game {
class new_game_window;

/** This window is where you define the properties of an AI player, before adding them to your game. */
class new_ai_player_window {
private:
  new_game_window *_new_game_window;
  fw::gui::Window *_wnd;

  bool on_ok_clicked(fw::gui::Widget *w);
  bool on_cancel_clicked(fw::gui::Widget *w);

public:
  new_ai_player_window();
  ~new_ai_player_window();

  void initialize(new_game_window *new_game_window);
  void show();
  void hide();
};

}

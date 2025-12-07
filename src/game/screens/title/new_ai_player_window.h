#pragma once

namespace fw{
namespace gui {
class Window;
class Widget;
} }

namespace game {
class NewGameWindow;

// This window is where you define the properties of an AI player, before adding them to your game.
class NewAIPlayerWindow {
private:
  NewGameWindow *new_game_window_;
  std::shared_ptr<fw::gui::Window> wnd_;

  bool on_ok_clicked(fw::gui::Widget &w);
  bool on_cancel_clicked(fw::gui::Widget &w);

public:
  NewAIPlayerWindow();
  ~NewAIPlayerWindow();

  void initialize(NewGameWindow *new_game_window);
  void show();
  void hide();
};

}

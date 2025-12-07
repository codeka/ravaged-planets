#pragma once

namespace fw {
namespace gui {
class Window;
class Widget;
}
}

namespace game {
class NewGameWindow;

// This is the main menu, it's got such things as the "New Game", "Join Game", etc buttons.
class MainMenuWindow {
private:
  NewGameWindow *new_game_window_;
  std::shared_ptr<fw::gui::Window> wnd_;
  bool exiting_;

  bool new_game_clicked(fw::gui::Widget &w);
  bool join_game_clicked(fw::gui::Widget &w);
  bool quit_clicked(fw::gui::Widget &w);
  bool editor_clicked(fw::gui::Widget &w);

public:
  MainMenuWindow();
  ~MainMenuWindow();
  void initialize(NewGameWindow *new_game_window);

  void update();
  void show();
  void hide();
};

}

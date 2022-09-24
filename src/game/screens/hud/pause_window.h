#pragma once

namespace fw {
namespace gui {
class Window;
class Widget;
}
}

namespace game {

/** The pause window displays when you press "Esc" to pause the game. */
class PauseWindow {
private:
  fw::gui::Window *wnd_;

  bool on_resume_clicked(fw::gui::Widget *w);
  bool on_exit_to_menu_clicked(fw::gui::Widget *w);
  bool on_exit_game_clicked(fw::gui::Widget *w);

public:
  PauseWindow();
  ~PauseWindow();
  void initialize();

  void show();
  void hide();
  bool is_visible() const;
};

extern PauseWindow *hud_pause;

}

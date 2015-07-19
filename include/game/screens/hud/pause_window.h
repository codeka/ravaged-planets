#pragma once

namespace fw {
namespace gui {
class window;
class widget;
}
}

namespace game {

/** The pause window displays when you press "Esc" to pause the game. */
class pause_window {
private:
  fw::gui::window *_wnd;

  bool on_resume_clicked(fw::gui::widget *w);
  bool on_exit_to_menu_clicked(fw::gui::widget *w);
  bool on_exit_game_clicked(fw::gui::widget *w);

public:
  pause_window();
  ~pause_window();
  void initialize();

  void show();
  void hide();
  bool is_visible() const;
};

extern pause_window *hud_pause;

}

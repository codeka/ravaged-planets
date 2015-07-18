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

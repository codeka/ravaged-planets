#pragma once

namespace fw {
namespace gui {
class window;
}

/** GUI view which shows a bit of debug information about the game (fps, num active particles, etc). */
class debug_view {
private:
  fw::gui::window *_wnd;
  float _time_to_update;

public:
  debug_view();
  ~debug_view();

  void initialize();
  void destroy();
  void update(float dt);
};

}
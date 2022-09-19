#pragma once

#include <framework/gui/window.h>

namespace fw {

/** GUI view which shows a bit of debug information about the game (fps, num active particles, etc). */
class debug_view {
private:
  fw::gui::Window *_wnd;
  float _time_to_update;

public:
  debug_view();
  ~debug_view();

  void initialize();
  void destroy();
  void update(float dt);
};

}
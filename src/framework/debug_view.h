#pragma once

#include <memory>

#include <framework/gui/window.h>

namespace fw {

/** GUI view which shows a bit of debug information about the game (fps, num active particles, etc). */
class DebugView {
private:
  std::shared_ptr<fw::gui::Window> wnd_;
  float time_to_update_;

public:
  DebugView();
  ~DebugView();

  void initialize();
  void destroy();
  void update(float dt);
};

}

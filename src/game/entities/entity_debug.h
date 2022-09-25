#pragma once

#include <framework/vector.h>
#include <framework/color.h>

namespace fw {
namespace gui {
class Window;
class Widget;
}
namespace sg {
class Scenegraph;
}
}

namespace ent {
class EntityManager;

// These are the different flags that apply to a single Entity.
enum EntityDebugFlags {
  // If this is set, we should render "steering vectors" which show how the Entity is currently steering.
  kDebugShowSteering = 1,

  kDebugMaxValue = 2
};


// This class contains some debug-related state. It registers the Ctrl+D key to enable/disable
// "Entity debugging" which shows things such as steering behaviours, pathing finding and so on.
class EntityDebug {
private:
  EntityManager *mgr_;
  fw::gui::Window *wnd_;

  bool just_shown_;

  void on_key_press(std::string keyname, bool is_down);
  bool on_show_steering_changed(fw::gui::Widget *w);

public:
  EntityDebug(EntityManager *mgr);
  ~EntityDebug();

  void initialize();

  /** Called each frame to update our state. */
  void update();
};

/**
 * This is a class that each Entity has access to and allows you to draw various lines and points
 * and so on that represent the various debugging information we can visualize.
 */
class EntityDebugView {
private:
  struct Line {
    fw::Vector from;
    fw::Vector to;
    fw::Color col;
  };
  std::vector<Line> lines_;

public:
  EntityDebugView();
  ~EntityDebugView();

  void add_line(fw::Vector const &from, fw::Vector const &to,
      fw::Color const &col);
  void add_circle(fw::Vector const &center, float radius,
      fw::Color const &col);

  void render(fw::sg::Scenegraph &scenegraph, fw::Matrix const &transform);
};

}

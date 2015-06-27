#pragma once

#include <framework/vector.h>
#include <framework/colour.h>

namespace fw {
namespace gui {
class window;
class widget;
}
namespace sg {
class scenegraph;
}
}

namespace ent {
class entity_manager;

/** These are the different flags that apply to a single entity. */
enum entity_debug_flags {
  /**
   * If this is set, we should render "steering vectors" which show how the entity is currently steering.
   */
  debug_show_steering = 1,

  debug_max_value = 2
};

/**
 * This class contains some debug-related state. It registers the Ctrl+D key to enable/disable
 * "entity debugging" which shows things such as steering behaviours, pathing finding and so on.
 */
class entity_debug {
private:
  entity_manager *_mgr;
  fw::gui::window *_wnd;

  bool _just_shown;

  void on_key_press(std::string keyname, bool is_down);
  bool on_show_steering_changed(fw::gui::widget *w);

public:
  entity_debug(entity_manager *mgr);
  ~entity_debug();

  void initialize();

  /** Called each frame to update our state. */
  void update();
};

/**
 * This is a class that each entity has access to and allows you to draw various lines and points
 * and so on that represent the various debugging information we can visualize.
 */
class entity_debug_view {
private:
  struct line {
    fw::vector from;
    fw::vector to;
    fw::colour col;
  };
  std::vector<line> _lines;

public:
  entity_debug_view();
  ~entity_debug_view();

  void add_line(fw::vector const &from, fw::vector const &to,
      fw::colour const &col);
  void add_circle(fw::vector const &centre, float radius,
      fw::colour const &col);

  void render(fw::sg::scenegraph &scenegraph, fw::matrix const &transform);
};

}

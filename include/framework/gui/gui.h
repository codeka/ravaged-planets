#pragma once

#include <vector>

namespace fw {
class graphics;

namespace gui {
class window;
class drawable_manager;

/**
 * This is the main entry point into the GUI subsystem. You get an instance of this class from \ref fw::framework.
 */
class gui {
private:
  fw::graphics *_graphics;
  drawable_manager *_drawable_manager;
  std::vector<window *> _top_level_windows;

public:
  gui();
  ~gui();

  /** Called to initialize the GUI system. */
  void initialize(fw::graphics *graphics);

  /** Called on the update thread to update the GUI. */
  void update(float dt);

  /** Called on the render thread to actually render the GUI. */
  void render();

  /** Creates a new top-level window, which you can add controls to. */
  window *create_window();

  /** Destroys the given window, unhooks any signals and removes it from the screen. */
  void destroy_window(window *wnd);

  int get_width() const;
  int get_height() const;

  inline drawable_manager *get_drawable_manager() const {
    return _drawable_manager;
  }
};

} }

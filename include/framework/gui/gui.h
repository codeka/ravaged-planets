#pragma once

#include <mutex>
#include <vector>

#define BOOST_BIND_NO_PLACEHOLDERS // so it doesn't auto-include _1, _2 etc.
#include <boost/signals2.hpp>

namespace fw {
class graphics;

namespace gui {
class widget;
class drawable_manager;

/**
 * This is the main entry point into the GUI subsystem. You get an instance of this class from \ref fw::framework.
 */
class gui {
private:
  fw::graphics *_graphics;
  drawable_manager *_drawable_manager;
  std::mutex _top_level_widget_mutex;
  std::vector<widget *> _top_level_widgets;
  std::vector<widget *> _pending_remove;
  widget *_widget_under_mouse;
  widget *_widget_mouse_down;
  widget *_focused;

  /** Gets the leaf-most widget at the given (x, y) coordinates, or null if there's no widget. */
  widget *get_widget_at(float x, float y);

public:
  gui();
  ~gui();

  /** Called to initialize the GUI system. */
  void initialize(fw::graphics *graphics);

  /** Called on the update thread to update the GUI. */
  void update(float dt);

  /** Called on the render thread to actually render the GUI. */
  void render();

  /** Register a new top-level widget. */
  void attach_widget(widget *widget);

  /** Destroys the given top-level widget, unhooks any signals and removes it from the screen. */
  void detach_widget(widget *widget);

  /** Give the specified widget input focus. Keystrokes will be sent to this widget only. */
  void focus(widget *widget);

  int get_width() const;
  int get_height() const;

  /** Injects a mouse button up/down event, returns true if we handled it or false if it should be passed through. */
  bool inject_mouse(int button, bool is_down, float x, float y);

  /** Injects a key press, returns true if we handled it or false if it should be passed through. */
  bool inject_key(int key, bool is_down);

  /** Global 'click' signal, fired whenever you click the mouse. Widget may be null if you clicked on no widget. */
  boost::signals2::signal<void(int button, bool is_down, widget *widget)> sig_click;

  inline drawable_manager *get_drawable_manager() const {
    return _drawable_manager;
  }
};

} }

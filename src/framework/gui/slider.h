#pragma once

#include <memory>
#include <string>

#include <framework/gui/widget.h>

namespace fw { namespace gui {
class gui;
class drawable;

/** Sliders let you drag a thumb from the left to the right to choose a numeric value. */
class slider : public widget {
protected:
  friend class slider_limits_property;
  friend class slider_on_update_property;
  friend class slider_value_property;

  std::shared_ptr<drawable> _line;
  std::shared_ptr<drawable> _thumb;
  int _min_value;
  int _max_value;
  int _curr_value;
  bool _dragging;
  std::function<void(int)> _on_update;

  void update_value(float mouse_x);

public:
  slider(gui *gui);
  virtual ~slider();

  static property *limits(int min_value, int max_value);
  static property *value(int curr_value);
  static property *on_update(std::function<void(int)> fn);

  void on_attached_to_parent(widget *parent);
  void on_mouse_out();
  void on_mouse_over();
  bool on_mouse_down(float x, float y);
  bool on_mouse_up(float x, float y);
  void on_mouse_move(float x, float y);
  void render();

  void set_limit(int min_value, int max_value);
  inline int get_min_value() const {
    return _min_value;
  }
  inline int get_max_value() const {
    return _max_value;
  }
  inline int get_curr_value() const {
    return _curr_value;
  }
};

} }

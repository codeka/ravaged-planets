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

  std::shared_ptr<drawable> _line;
  std::shared_ptr<drawable> _thumb;
  int _min_value;
  int _max_value;
  int _curr_value;

public:
  slider(gui *gui);
  virtual ~slider();

  static property *limits(int min_value, int max_value);

  void on_attached_to_parent(widget *parent);
  void on_mouse_out();
  void on_mouse_over();
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

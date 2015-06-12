#pragma once

#include <vector>
#include <framework/gui/property.h>

namespace fw { namespace gui {
class gui;
class widget;

/**
 * Represents a dimension: either an (x,y) coordinate or a width/height.
 */
class dimension {
public:
  dimension();
  virtual ~dimension();

  virtual float get_value(float parent_value) = 0;
};

class pixel_dimension : public dimension {
private:
  float _value;

public:
  pixel_dimension(float value);
  virtual ~pixel_dimension();

  float get_value(float parent_value);
};

class percent_dimension : public dimension {
private:
  float _value;

public:
  percent_dimension(float value);
  virtual ~percent_dimension();

  float get_value(float parent_value);
};

class sum_dimension : public dimension {
private:
  std::shared_ptr<dimension> _one;
  std::shared_ptr<dimension> _two;

public:
  sum_dimension(std::shared_ptr<dimension> one, std::shared_ptr<dimension> two);
  virtual ~sum_dimension();

  float get_value(float parent_value);
};

inline std::shared_ptr<dimension> px(float value) {
  return std::shared_ptr<dimension>(new pixel_dimension(value));
}

inline std::shared_ptr<dimension> pct(float value) {
  return std::shared_ptr<dimension>(new percent_dimension(value));
}

inline std::shared_ptr<dimension> sum(std::shared_ptr<dimension> one, std::shared_ptr<dimension> two) {
  return std::shared_ptr<dimension>(new sum_dimension(one, two));
}

/**
 * This is the base class of all widgets in the GUI. A widget has a specific position within it's parent, size and
 * so on.
 */
class widget {
protected:
  friend class widget_position_property;
  friend class widget_size_property;
  friend class widget_click_property;
  friend class widget_visible_property;

  gui *_gui;
  widget *_parent;
  std::vector<widget *> _children;
  std::shared_ptr<dimension> _x;
  std::shared_ptr<dimension> _y;
  std::shared_ptr<dimension> _width;
  std::shared_ptr<dimension> _height;
  bool _visible;
  std::function<bool(widget *)> _on_click;

public:
  widget(gui *gui);
  virtual ~widget();

  static property *position(std::shared_ptr<dimension> x, std::shared_ptr<dimension> y);
  static property *size(std::shared_ptr<dimension> width, std::shared_ptr<dimension> height);
  static property *click(std::function<bool(widget *)> on_click);
  static property *visible(bool visible);

  void attach_child(widget *child);
  void detach_child(widget *child);
  virtual void on_attached_to_parent(widget *parent);

  virtual void render();

  /** Called when the mouse moves out of this widget. */
  virtual void on_mouse_out() {
  }

  /** Called when the mouse moves over this widget. */
  virtual void on_mouse_over() {
  }

  virtual bool on_mouse_down();
  virtual bool on_mouse_up();

  /**
   * Gets the child widget at the given (x,y). If the point is outside our bounding box, the null is returned. If
   * none of our children are contained within the given (x,y) then \code this is returned.
   */
  widget *get_child_at(float x, float y);

  float get_top();
  float get_left();
  float get_width();
  float get_height();

  bool is_visible() const {
    return _visible;
  }
  void set_visible(bool visible) {
    _visible = visible;
  }
};

} }

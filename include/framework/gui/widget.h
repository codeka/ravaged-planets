#pragma once

#include <vector>
#include <framework/gui/property.h>

namespace fw { namespace gui {
class gui;
class widget;

/**
 * Represents either an absolute (pixel) or relative (percent) dimension. Used to position and measure widgets.
 */
class dimension {
public:
  enum kind {
    pixels,
    percent
  };

  dimension();
  dimension(kind kind, float value);

  kind _kind;
  float _value;
};

inline dimension px(float value) {
  return dimension(dimension::pixels, value);
}

inline dimension pct(float value) {
  return dimension(dimension::percent, value);
}

class position_property : public property {
private:
  dimension _x;
  dimension _y;
public:
  position_property(dimension const &x, dimension const &y);
  void apply(widget *widget);
};

class size_property : public property {
private:
  dimension _width;
  dimension _height;
public:
  size_property(dimension const &width, dimension const &height);
  void apply(widget *widget);
};

/**
 * This is the base class of all widgets in the GUI. A widget has a specific position within it's parent, size and
 * so on.
 */
class widget {
protected:
  friend class position_property;
  friend class size_property;
  friend class widget_click_property;

  gui *_gui;
  widget *_parent;
  std::vector<widget *> _children;
  dimension _x;
  dimension _y;
  dimension _width;
  dimension _height;
  std::function<bool(widget *)> _on_click;

public:
  widget(gui *gui);
  virtual ~widget();

  static inline property *position(dimension const &x, dimension const &y) {
    return new position_property(x, y);
  }
  static inline property *size(dimension const &width, dimension const &height) {
    return new size_property(width, height);
  }
  static property *click(std::function<bool(widget *)> on_click);

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
};

} }

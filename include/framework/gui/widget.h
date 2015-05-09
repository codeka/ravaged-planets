#pragma once

#include <framework/gui/property.h>

namespace fw { namespace gui {
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

private:
  kind _kind;
  float _value;

public:
  dimension();
  dimension(kind kind, float value);

  /**
   * Resolve the given dimension for the given widget. We make need to query the parent to get percents correct.
   */
  float resolve(widget *widget);
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

  dimension _x;
  dimension _y;
  dimension _width;
  dimension _height;

public:
  widget();
  virtual ~widget();

  static inline property *position(dimension const &x, dimension const &y) {
    return new position_property(x, y);
  }
  static inline property *size(dimension const &width, dimension const &height) {
    return new size_property(width, height);
  }

};

} }

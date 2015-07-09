#pragma once

#include <vector>
#include <boost/any.hpp>

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

/** Base class for properties that can be added to buildable objects. */
class property {
public:
  inline virtual ~property() { }

  virtual void apply(widget *widget) = 0;
};

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
  friend class widget_id_property;
  friend class widget_data_property;

  gui *_gui;
  widget *_parent;
  int _id;
  std::vector<widget *> _children;
  std::shared_ptr<dimension> _x;
  std::shared_ptr<dimension> _y;
  std::shared_ptr<dimension> _width;
  std::shared_ptr<dimension> _height;
  bool _visible;
  bool _focused;
  std::function<bool(widget *)> _on_click;
  boost::any _data;

public:
  widget(gui *gui);
  virtual ~widget();

  static property *position(std::shared_ptr<dimension> x, std::shared_ptr<dimension> y);
  static property *size(std::shared_ptr<dimension> width, std::shared_ptr<dimension> height);
  static property *click(std::function<bool(widget *)> on_click);
  static property *visible(bool visible);
  static property *id(int id);
  static property *data(boost::any const &data);

  void attach_child(widget *child);
  void detach_child(widget *child);
  void clear_children();
  virtual void on_attached_to_parent(widget *parent);

  virtual void on_focus_gained();
  virtual void on_focus_lost();

  /** Called when a key is pressed. Only called when this widget has focus. */
  virtual bool on_key(int key, bool is_down) {
    return false;
  }

  /** Override this if you want your widget to accept input focus. */
  virtual bool can_focus() const {
    return false;
  }

  virtual void update(float dt);

  /** Called just before render. You should not override this, it define the scissor rectangle and stuff like that. */
  bool prerender();
  virtual void render();
  /** Called just after render. You should not override this, it define the scissor rectangle and stuff like that. */
  void postrender();

  /** Called when the mouse moves out of this widget. */
  virtual void on_mouse_out() {
  }

  /** Called when the mouse moves over this widget. */
  virtual void on_mouse_over() {
  }

  /** Called when the mouse moves over us. (x,y) is relative to this widget's origin. */
  virtual void on_mouse_move(float x, float y) {
  }

  /** Called when the mouse is pressed down, (x,y) is relative to this widget's origin. */
  virtual bool on_mouse_down(float x, float y);

  /** Called when the mouse is released, (x,y) is relative to this widget's origin. */
  virtual bool on_mouse_up(float x, float y);

  /**
   * Gets the child widget at the given (x,y). If the point is outside our bounding box, then null is returned. If
   * none of our children are contained within the given (x,y) then \code this is returned.
   */
  widget *get_child_at(float x, float y);

  /** Searches the heirarchy for the widget with the given id. */
  widget *find(int id);

  widget *get_parent();

  template<typename T>
  inline T *find(int id) {
    return dynamic_cast<T *>(find(id));
  }

  /** Returns true if the given widget is a child (or a child of a child...) of us. */
  bool is_child(widget *w);

  float get_top();
  void set_top(std::shared_ptr<dimension> top);
  float get_left();
  void set_left(std::shared_ptr<dimension> left);
  float get_width();
  void set_width(std::shared_ptr<dimension> width);
  float get_height();
  void set_height(std::shared_ptr<dimension> height);

  boost::any const &get_data() const;
  void set_data(boost::any const &data);

  bool is_visible() const {
    return _visible;
  }
  void set_visible(bool visible);
};

} }

#pragma once

#include <vector>
#include <boost/any.hpp>
#define BOOST_BIND_NO_PLACEHOLDERS // so it doesn't auto-include _1, _2 etc.
#include <boost/signals2.hpp>

namespace fw::gui {
class Gui;
class Widget;

// Represents a dimension: either an (x,y) coordinate or a width/height.
class Dimension {
public:
  Dimension();
  virtual ~Dimension();

  virtual float get_value(fw::gui::Widget *w, float parent_value) = 0;
};

class PixelDimension : public Dimension {
private:
  float value_;

public:
  PixelDimension(float value);
  virtual ~PixelDimension();

  float get_value(fw::gui::Widget *w, float parent_value);
};

class PercentDimension : public Dimension {
private:
  float value_;

public:
  PercentDimension(float value);
  virtual ~PercentDimension();

  float get_value(fw::gui::Widget *w, float parent_value);
};

class SumDimension : public Dimension {
private:
  std::shared_ptr<Dimension> one_;
  std::shared_ptr<Dimension> two_;

public:
  SumDimension(std::shared_ptr<Dimension> one, std::shared_ptr<Dimension> two);
  virtual ~SumDimension();

  float get_value(fw::gui::Widget *w, float parent_value);
};

enum OtherDimension {
  kTop,
  kLeft,
  kWidth,
  kHeight,
};

/** A dimension that's defined as a fraction of another dimension (e.g. width = 50% of the height, etc). */
class FractionDimension : public Dimension {
private:
  int other_widget_id_;
  OtherDimension other_dimension_;
  float fraction_;

public:
  FractionDimension(OtherDimension dim, float fraction);
  FractionDimension(int other_widget_id, OtherDimension dim, float fraction);
  virtual ~FractionDimension();

  float get_value(fw::gui::Widget *w, float parent_value);
};

inline std::shared_ptr<Dimension> px(float value) {
  return std::shared_ptr<Dimension>(new PixelDimension(value));
}

inline std::shared_ptr<Dimension> pct(float value) {
  return std::shared_ptr<Dimension>(new PercentDimension(value));
}

inline std::shared_ptr<Dimension> sum(std::shared_ptr<Dimension> one, std::shared_ptr<Dimension> two) {
  return std::shared_ptr<Dimension>(new SumDimension(one, two));
}

inline std::shared_ptr<Dimension> fract(OtherDimension dimen, float fraction) {
  return std::shared_ptr<Dimension>(new FractionDimension(dimen, fraction));
}

inline std::shared_ptr<Dimension> fract(int other_widget_id, OtherDimension dimen, float fraction) {
  return std::shared_ptr<Dimension>(new FractionDimension(other_widget_id, dimen, fraction));
}

// Base class for properties that can be added to buildable objects.
class Property {
public:
  inline virtual ~Property() { }

  virtual void apply(Widget *widget) = 0;
};

// This is the base class of all widgets in the GUI. A widget has a specific position within it's parent, size and
// so on.
class Widget {
protected:
  friend class WidgetPositionProperty;
  friend class WidgetSizeProperty;
  friend class WidgetClickProperty;
  friend class WidgetVisibleProperty;
  friend class WidgetIdProperty;
  friend class WidgetDataProperty;
  friend class WidgetEnabledProperty;

  Gui *gui_;
  Widget *parent_;
  int id_;
  std::vector<Widget *> children_;
  std::shared_ptr<Dimension> x_;
  std::shared_ptr<Dimension> y_;
  std::shared_ptr<Dimension> width_;
  std::shared_ptr<Dimension> height_;
  bool visible_;
  bool focused_;
  bool enabled_;
  std::function<bool(Widget *)> on_click_;
  boost::any data_;

public:
  Widget(Gui *gui);
  virtual ~Widget();

  static Property *position(std::shared_ptr<Dimension> x, std::shared_ptr<Dimension> y);
  static Property *size(std::shared_ptr<Dimension> width, std::shared_ptr<Dimension> height);
  static Property *click(std::function<bool(Widget *)> on_click);
  static Property *visible(bool visible);
  static Property *id(int id);
  static Property *data(boost::any const &data);
  static Property *enabled(bool enabled);

  void attach_child(Widget *child);
  void detach_child(Widget *child);
  void clear_children();
  virtual void on_attached_to_parent(Widget *parent);

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

  /** Gets the name of the cursor we want to display when the mouse is over us. */
  virtual std::string get_cursor_name() const;

  virtual void update(float dt);

  /** Called just before render. You should not override this, it define the scissor rectangle and stuff like that. */
  bool prerender();
  virtual void render();
  /** Called just after render. You should not override this, it define the scissor rectangle and stuff like that. */
  void postrender();

  /** Signalled when the mouse moves out of this widget. */
  boost::signals2::signal<void()> sig_mouse_out;

  /** Signalled when the mouse moves over this widget. */
  boost::signals2::signal<void()> sig_mouse_over;

  /** Signalled when the mouse moves over us. (x,y) is relative to this widget's origin. */
  boost::signals2::signal<void(float x, float y)> sig_mouse_move;

  /** Called when the mouse is pressed down, (x,y) is relative to this widget's origin. */
  virtual bool on_mouse_down(float x, float y);

  /** Called when the mouse is released, (x,y) is relative to this widget's origin. */
  virtual bool on_mouse_up(float x, float y);

  /**
   * Gets the child widget at the given (x,y). If the point is outside our bounding box, then null is returned. If
   * none of our children are contained within the given (x,y) then \code this is returned.
   */
  Widget *get_child_at(float x, float y);

  /** Searches the heirarchy for the widget with the given id. */
  Widget *find(int id);

  Widget *get_parent();

  /** Gets the root parent, that is, keep looking up the parent chain until you find one that has a null parent. */
  Widget *get_root();

  template<typename T>
  inline T *find(int id) {
    return dynamic_cast<T *>(find(id));
  }

  /** Returns true if the given widget is a child (or a child of a child...) of us. */
  bool is_child(Widget *w);

  int get_id() {
    return id_;
  }

  float get_top();
  void set_top(std::shared_ptr<Dimension> top);
  float get_left();
  void set_left(std::shared_ptr<Dimension> left);
  float get_width();
  void set_width(std::shared_ptr<Dimension> width);
  float get_height();
  void set_height(std::shared_ptr<Dimension> height);

  boost::any const &get_data() const;
  void set_data(boost::any const &data);

  void set_on_click(std::function<bool(Widget *)> on_click) {
    on_click_ = on_click;
  }

  bool is_enabled() const {
    return enabled_;
  }
  void set_enabled(bool enabled);

  bool is_visible() const {
    return visible_;
  }
  void set_visible(bool visible);
};

}

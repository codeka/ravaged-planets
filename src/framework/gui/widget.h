#pragma once

#include <any>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <framework/math.h>
#include <framework/misc.h>
#include <framework/signals.h>

namespace fw::gui {
class Widget;

template <typename T>
class Builder;

// Base class for properties that can be added to buildable objects.
class Property {
public:
  inline virtual ~Property() { }

  virtual void apply(Widget &widget) = 0;
};

template<typename T>
concept IsSubclassOfWidget = std::is_base_of_v<Widget, T>;

// This is passed to the Measure() function of a widget, it tells the widget how much space it has
// to work with and how it should interpret that space (e.g. "you must be exactly this big", "you
// can be as big as you want up to this size", etc).
class MeasureSpec {
 public:
  enum Mode {
    // The parent has determined an exact size for the child. The child is required to use this 
    // size, and the parent is not going to look at the child's measured size at all (except to
    // verify it matches).
    kExactly,
    // The child can be as big as it wants up to the specified size. The parent is going to look
    // at the child's measured size and decide how much space to actually give it.
    kAtMost,  
    // The parent doesn't care how big the child is. The child can be whatever size it wants.
    kUnspecified,
  };
  Mode mode;
	float size;

	inline MeasureSpec(Mode mode, float size) : mode(mode), size(size) {}

  inline static MeasureSpec Exactly(float size) {
    return MeasureSpec(Mode::kExactly, size);
	}
  inline static MeasureSpec AtMost(float size) {
    return MeasureSpec(Mode::kAtMost, size);
  }
  inline static MeasureSpec Unspecified() {
    return MeasureSpec(Mode::kUnspecified, 0);
	}
};

// This is the result of a Measure() call, it contains the measured width and height of the widget.
class MeasuredSize{
 public:
  float width;
  float height;
	inline MeasuredSize(float width, float height) : width(width), height(height) {}
};

// Base class for how a widget wants to lay itself out inside a parent. Each parent will have it's
// own subclass of this, which you can customize using the Builder.
class LayoutParams {
public:
  enum Mode {
    // The widget is a fixed size.
    kFixed,
    // The widget wants to match the size of it's parent.
    kMatchParent,
    // The widget wants to be as big as it needs to fix its content.
		kWrapContent,
  };

  Mode width_mode;
  float width;

	Mode height_mode;
  float height;

	float top_margin;
  float right_margin;
	float bottom_margin;
  float left_margin;

  LayoutParams() : width_mode(Mode::kFixed), width(0), height_mode(Mode::kFixed), height(0),
    top_margin(0), right_margin(0), bottom_margin(0), left_margin(0) {
	}
  LayoutParams(Mode width_mode, float width, Mode height_mode, float height) :
		width_mode(width_mode), width(width), height_mode(height_mode), height(height),
    top_margin(0), right_margin(0), bottom_margin(0), left_margin(0) {
	}
};

// This is the base class of all widgets in the GUI. A widget has a specific position within it's
// parent, size and so on.
class Widget : public std::enable_shared_from_this<Widget> {
protected:
  friend class WidgetPositionProperty;
  friend class WidgetSizeProperty;
  friend class WidgetClickProperty;
  friend class WidgetVisibleProperty;
  friend class WidgetIdProperty;
  friend class WidgetNameProperty;
  friend class WidgetDataProperty;
  friend class WidgetEnabledProperty;

  std::weak_ptr<Widget> parent_;
  std::shared_ptr<LayoutParams> layout_params_ = std::make_shared<LayoutParams>();
  int id_ = 0;
  std::string name_;
  std::vector<std::shared_ptr<Widget>> children_;
  float x_ = 0.f;
  float y_ = 0.f;
  float width_ = 0.f;
  float height_ = 0.f;
  bool visible_ = true;
  bool focused_ = false;
  bool enabled_ = true;
  std::function<bool(Widget&)> on_click_;
  std::any data_;
	MeasuredSize measured_size_;

  MeasuredSize ResolveSize(
      MeasureSpec width_spec,
      float measured_width,
      MeasureSpec height_spec,
      float measured_height);

  // Called when something changes that requires a re-layout (e.g. children added/remove, size
  // changes, layout params change, etc). We simply pass it up the chain to the parent. The Window
  // class is expected to be the root and actually coordinate the re-layout.
  virtual void RequestLayout() {
    auto parent = parent_.lock();
    if (parent) {
      parent->RequestLayout();
		}
  }

  MeasuredSize Measure(MeasureSpec width_spec, MeasureSpec height_spec);
  MeasuredSize MeasureChild(
      MeasureSpec parent_width_spec,
      float width_used,
      MeasureSpec parent_height_spec,
      float height_used);

  virtual MeasuredSize OnMeasure(MeasureSpec width_spec, MeasureSpec height_spec);

	// Performa a layout of this widget. If you need to customize this behavior, you should override
  // OnLayout() which is called by this method.
  void PerformLayout(float top, float right, float bottom, float left);

  virtual void OnLayout(float top, float right, float bottom, float left);

public:
  Widget();
  virtual ~Widget();

  static std::unique_ptr<Property> width(LayoutParams::Mode width_mode, float width);
  static std::unique_ptr<Property> height(LayoutParams::Mode height_mode, float height);
	static std::unique_ptr<Property> margin(float top, float right, float bottom, float left);

  static std::unique_ptr<Property> click(std::function<bool(Widget &)> on_click);
  static std::unique_ptr<Property> visible(bool visible);
  static std::unique_ptr<Property> id(int id);
  static std::unique_ptr<Property> name(std::string_view name);
  static std::unique_ptr<Property> data(std::any const &data);
  static std::unique_ptr<Property> enabled(bool enabled);

  void AttachChild(std::shared_ptr<Widget> child);

  template<IsSubclassOfWidget T>
  void AttachChild(std::shared_ptr<T> child) {
    AttachChild(std::dynamic_pointer_cast<Widget>(child));
  }

  template<IsSubclassOfWidget T>
  void AttachChild(Builder<T> &child);

  void DetachChild(std::shared_ptr<Widget> child);
  void ClearChildren();
  virtual void OnAttachedToParent(Widget &parent);

  virtual std::shared_ptr<LayoutParams> CreateLayoutParams() {
    return std::make_shared<LayoutParams>(
      LayoutParams::Mode::kFixed, 0.f, LayoutParams::Mode::kFixed, 0.f);
  }
  std::shared_ptr<LayoutParams> CreateLayoutParams(
      LayoutParams::Mode width_mode, float width,
      LayoutParams::Mode height_mode, float height) {
		std::shared_ptr<LayoutParams> lp = CreateLayoutParams();
		lp->width_mode = width_mode;
		lp->width = width;
		lp->height_mode = height_mode;
		lp->height = height;
    return lp;
  }

  inline MeasuredSize get_measured_size() const {
    return measured_size_;
	}

  // Gets the layout params for this widget. This will be null if we're not attached to a parent.
  inline std::shared_ptr<LayoutParams> get_layout_params() const {
    return layout_params_;
	}

  virtual void on_focus_gained();
  virtual void on_focus_lost();

  /** Called when a key is pressed. Only called when this widget has focus. */
  virtual bool on_key(int key, bool is_down) {
    return false;
  }

  /** Override this if you want your widget to accept Input focus. */
  virtual bool can_focus() const {
    return false;
  }

  /** Gets the name of the cursor we want to display when the mouse is over us. */
  virtual std::string get_cursor_name() const;

  virtual void update(float dt);

  /**
   * Called just before render. You should not override this, it define the scissor Rectangle and
   * stuff like that.
   */
  bool prerender();
  virtual void render();
  /**
   * Called just after render. You should not override this, it define the scissor Rectangle and
   * stuff like that.
   */
  void postrender();

  /** Signalled when the mouse moves out of this widget. */
  fw::Signal<> sig_mouse_out;

  /** Signalled when the mouse moves over this widget. */
  fw::Signal<> sig_mouse_over;

  /** Signalled when the mouse moves over us. (x,y) is relative to this widget's origin. */
  fw::Signal<float /*x*/, float /*y*/> sig_mouse_move;

  /** Called when the mouse is pressed down, (x,y) is relative to this widget's origin. */
  virtual bool on_mouse_down(float x, float y);

  /** Called when the mouse is released, (x,y) is relative to this widget's origin. */
  virtual bool on_mouse_up(float x, float y);

  /**
   * Gets the child widget at the given (x,y). If the point is outside our bounding box, then null
   * is returned. If none of our children are contained within the given (x,y) then \code this is
   * returned.
   */
  std::shared_ptr<Widget> GetChildAt(float x, float y);

  /** Searches the heirarchy for the widget with the given id. */
  std::shared_ptr<Widget> Find(int id);

  template<typename T>
  inline std::shared_ptr<T> Find(int id) {
    return std::dynamic_pointer_cast<T>(Find(id));
  }

  std::shared_ptr<Widget> get_parent();

  /**
   * Gets the root parent, that is, keep looking up the parent chain until you find one that has a
   * null parent.
   */
  std::shared_ptr<Widget> get_root();

  /** Returns true if the given widget is a child (or a child of a child...) of us. */
  bool IsChild(std::shared_ptr<Widget const> w) const;
  bool IsChild(Widget const &w) const;

  int get_id() {
    return id_;
  }
  std::string get_name() {
    return name_;
	}

  // Get the position of this widget in screen coordinates.
  fw::Point GetScreenPosition();

	// Get the rectangle bounding box of this widget in screen coordinates.
	fw::Rectangle<float> GetScreenRect();

  inline float get_x() {
    return x_;
  }
  inline float get_y() {
    return y_;
  }
  inline float get_width() {
    return width_;
	}
  inline float get_height() {
    return height_;
	}

  std::any const &get_data() const;
  void set_data(std::any const &data);

  void set_on_click(std::function<bool(Widget &)> on_click) {
    on_click_ = on_click;
  }

  bool is_enabled() const {
    return enabled_;
  }
  void set_enabled(bool enabled);

  bool is_visible() const {
    return visible_;
  }
  virtual void set_visible(bool visible);
};

}

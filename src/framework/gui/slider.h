#pragma once

#include <memory>
#include <string>

#include <framework/gui/widget.h>

namespace fw::gui {
class Gui;
class Drawable;

// Sliders let you drag a thumb from the left to the right to choose a numeric value.
class Slider : public Widget {
protected:
  friend class SliderLimitsProperty;
  friend class SliderOnUpdateProperty;
  friend class SliderValueProperty;

  std::shared_ptr<Drawable> line_;
  std::shared_ptr<Drawable> thumb_;
  int min_value_;
  int max_value_;
  int curr_value_;
  bool dragging_;
  std::function<void(int)> on_update_;

  void update_value(float mouse_x);

public:
  Slider(Gui *gui);
  virtual ~Slider();

  static Property *limits(int min_value, int max_value);
  static Property *value(int curr_value);
  static Property *on_update(std::function<void(int)> fn);

  void on_attached_to_parent(Widget *parent);
  void on_mouse_out();
  void on_mouse_over();
  bool on_mouse_down(float x, float y);
  bool on_mouse_up(float x, float y);
  void on_mouse_move(float x, float y);
  void render();

  void set_limit(int min_value, int max_value);
  inline int get_min_value() const {
    return min_value_;
  }
  inline int get_max_value() const {
    return max_value_;
  }
  inline int get_curr_value() const {
    return curr_value_;
  }
};

}

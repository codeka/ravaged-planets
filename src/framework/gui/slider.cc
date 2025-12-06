
#include <framework/framework.h>
#include <framework/logging.h>
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/slider.h>

namespace fw::gui {

static const int THUMB_WIDTH = 9;
static const int THUMB_HEIGHT = 18;

// Property that sets the limits of the slider.
class SliderLimitsProperty : public Property {
private:
  int min_value_;
  int max_value_;
public:
  SliderLimitsProperty(int min_value, int max_value) :
    min_value_(min_value), max_value_(max_value) {
  }

  void apply(Widget &widget) override {
    Slider &slider = dynamic_cast<Slider &>(widget);
    slider.min_value_ = min_value_;
    slider.max_value_ = max_value_;
  }
};

/** Property that sets the current ParticleRotation of the slider. */
class SliderValueProperty : public Property {
private:
  int curr_value_;
public:
  SliderValueProperty(int curr_value) :
      curr_value_(curr_value) {
  }

  void apply(Widget &widget) override {
    Slider &slider = dynamic_cast<Slider &>(widget);
    slider.curr_value_ = curr_value_;
  }
};

/** Property that sets the on_update callback for the slider. */
class SliderOnUpdateProperty : public Property {
private:
  std::function<void(int)> on_update_;
public:
  SliderOnUpdateProperty(std::function<void(int)> on_update) :
    on_update_(on_update) {
  }

  void apply(Widget &widget) override {
    Slider &slider = dynamic_cast<Slider &>(widget);
    slider.on_update_ = on_update_;
  }
};

//-----------------------------------------------------------------------------

Slider::Slider(Gui *gui) : Widget(gui), min_value_(0), max_value_(100), curr_value_(0), dragging_(false) {
}

Slider::~Slider() {
}

std::unique_ptr<Property> Slider::limits(int min_value, int max_value) {
  return std::make_unique<SliderLimitsProperty>(min_value, max_value);
}

std::unique_ptr<Property> Slider::value(int curr_value) {
  return std::make_unique<SliderValueProperty>(curr_value);
}

std::unique_ptr<Property> Slider::on_update(std::function<void(int)> on_update) {
  return std::make_unique<SliderOnUpdateProperty>(on_update);
}

void Slider::on_attached_to_parent(Widget *parent) {
  StateDrawable *bkgnd = new StateDrawable();
  bkgnd->add_drawable(
      StateDrawable::kNormal, gui_->get_drawable_manager().get_drawable("slider_thumb_normal"));
  bkgnd->add_drawable(
      StateDrawable::kHover, gui_->get_drawable_manager().get_drawable("slider_thumb_hover"));
  thumb_ = std::shared_ptr<Drawable>(bkgnd);
  line_ = gui_->get_drawable_manager().get_drawable("slider_line");
}

void Slider::on_mouse_out() {
  std::shared_ptr<StateDrawable> drawable = std::dynamic_pointer_cast<StateDrawable>(thumb_);
  drawable->set_current_state(StateDrawable::kNormal);
}

void Slider::on_mouse_over() {
  std::shared_ptr<StateDrawable> drawable = std::dynamic_pointer_cast<StateDrawable>(thumb_);
  drawable->set_current_state(StateDrawable::kHover);
}

bool Slider::on_mouse_down(float x, float y) {
  update_value(x);
  dragging_ = true;
  return true;
}

bool Slider::on_mouse_up(float x, float y) {
  dragging_ = false;
  return true;
}

void Slider::on_mouse_move(float x, float y) {
  if (dragging_) {
    update_value(x);
  }
}

void Slider::update_value(float mouse_x) {
  int new_value;
  float width = get_width();
  if (mouse_x > width - (THUMB_WIDTH / 2)) {
    new_value = max_value_;
  } else if (mouse_x <= (THUMB_WIDTH / 2)) {
    new_value = min_value_;
  } else {
    float fraction = (mouse_x - (THUMB_WIDTH / 2)) / (get_width() - THUMB_WIDTH);
    new_value = min_value_ + static_cast<int>(fraction * (max_value_ - min_value_));
  }

  if (new_value != curr_value_) {
    curr_value_ = new_value;
    if (on_update_) {
      on_update_(curr_value_);
    }
  }
}

void Slider::render() {
  int left = get_left();
  int top = get_top();
  int height = get_height();
  int width = get_width();
  line_->render(left, top + (height / 2), width, 1);

  float thumb_offset = (get_width() - THUMB_WIDTH) * (curr_value_ - min_value_) / (max_value_ - min_value_);
  thumb_->render(left + thumb_offset, top + (height / 2) - (THUMB_HEIGHT / 2), THUMB_WIDTH, THUMB_HEIGHT);
}

}


#include <framework/framework.h>
#include <framework/logging.h>
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/slider.h>

namespace fw { namespace gui {

static const int THUMB_WIDTH = 9;
static const int THUMB_HEIGHT = 18;

/** Property that sets the limits of the slider. */
class slider_limits_property : public property {
private:
  int _min_value;
  int _max_value;
public:
  slider_limits_property(int min_value, int max_value) :
      _min_value(min_value), _max_value(max_value) {
  }

  void apply(widget *widget) {
    slider *sldr = dynamic_cast<slider *>(widget);
    sldr->_min_value = _min_value;
    sldr->_max_value = _max_value;
  }
};

/** Property that sets the current value of the slider. */
class slider_value_property : public property {
private:
  int _curr_value;
public:
  slider_value_property(int curr_value) :
      _curr_value(curr_value) {
  }

  void apply(widget *widget) {
    slider *sldr = dynamic_cast<slider *>(widget);
    sldr->_curr_value = _curr_value;
  }
};

/** Property that sets the on_update callback for the slider. */
class slider_on_update_property : public property {
private:
  std::function<void(int)> _on_update;
public:
  slider_on_update_property(std::function<void(int)> on_update) :
    _on_update(on_update) {
  }

  void apply(widget *widget) {
    slider *sldr = dynamic_cast<slider *>(widget);
    sldr->_on_update = _on_update;
  }
};

//-----------------------------------------------------------------------------

slider::slider(gui *gui) : widget(gui), _min_value(0), _max_value(100), _curr_value(0), _dragging(false) {
}

slider::~slider() {
}

property *slider::limits(int min_value, int max_value) {
  return new slider_limits_property(min_value, max_value);
}

property *slider::value(int curr_value) {
  return new slider_value_property(curr_value);
}

property *slider::on_update(std::function<void(int)> on_update) {
  return new slider_on_update_property(on_update);
}

void slider::on_attached_to_parent(widget *parent) {
  state_drawable *bkgnd = new state_drawable();
  bkgnd->add_drawable(state_drawable::normal, _gui->get_drawable_manager()->get_drawable("slider_thumb_normal"));
  bkgnd->add_drawable(state_drawable::hover, _gui->get_drawable_manager()->get_drawable("slider_thumb_hover"));
  _thumb = std::shared_ptr<drawable>(bkgnd);
  _line = _gui->get_drawable_manager()->get_drawable("slider_line");
}

void slider::on_mouse_out() {
  std::shared_ptr<state_drawable> drawable = std::dynamic_pointer_cast<state_drawable>(_thumb);
  drawable->set_current_state(state_drawable::normal);
}

void slider::on_mouse_over() {
  std::shared_ptr<state_drawable> drawable = std::dynamic_pointer_cast<state_drawable>(_thumb);
  drawable->set_current_state(state_drawable::hover);
}

bool slider::on_mouse_down(float x, float y) {
  update_value(x);
  _dragging = true;
  return true;
}

bool slider::on_mouse_up(float x, float y) {
  _dragging = false;
  return true;
}

void slider::on_mouse_move(float x, float y) {
  if (_dragging) {
    update_value(x);
  }
}

void slider::update_value(float mouse_x) {
  int new_value;
  float width = get_width();
  if (mouse_x > width - (THUMB_WIDTH / 2)) {
    new_value = _max_value;
  } else if (mouse_x <= (THUMB_WIDTH / 2)) {
    new_value = _min_value;
  } else {
    float fraction = (mouse_x - (THUMB_WIDTH / 2)) / (get_width() - THUMB_WIDTH);
    new_value = _min_value + static_cast<int>(fraction * (_max_value - _min_value));
  }

  if (new_value != _curr_value) {
    _curr_value = new_value;
    if (_on_update) {
      _on_update(_curr_value);
    }
  }
}

void slider::render() {
  int left = get_left();
  int top = get_top();
  int height = get_height();
  int width = get_width();
  _line->render(left, top + (height / 2), width, 1);

  float thumb_offset = (get_width() - THUMB_WIDTH) * (_curr_value - _min_value) / (_max_value - _min_value);
  _thumb->render(left + thumb_offset, top + (height / 2) - (THUMB_HEIGHT / 2), THUMB_WIDTH, THUMB_HEIGHT);
}

} }

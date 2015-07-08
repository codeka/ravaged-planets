
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/slider.h>

#include <framework/framework.h>

namespace fw { namespace gui {

/** Property that sets the background of the button. */
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

//-----------------------------------------------------------------------------

slider::slider(gui *gui) : widget(gui), _min_value(0), _max_value(100), _curr_value(0) {
}

slider::~slider() {
}

property *slider::limits(int min_value, int max_value) {
  return new slider_limits_property(min_value, max_value);
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

void slider::render() {
  int left = get_left();
  int top = get_top();
  int height = get_height();
  _line->render(left, top + (height / 2), get_width(), 1);

  _thumb->render(left, top + (height / 2) - 9, 9, 18);
}

} }

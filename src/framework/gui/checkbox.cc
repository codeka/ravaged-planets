
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/checkbox.h>

#include <framework/framework.h>
#include <framework/font.h>


namespace fw { namespace gui {

/** Property that sets the text of the checkbox. */
class checkbox_text_property : public property {
private:
  std::string _text;
public:
  checkbox_text_property(std::string const &text) :
      _text(text) {
  }

  void apply(widget *widget) {
    checkbox *chbx = dynamic_cast<checkbox *>(widget);
    chbx->_text = _text;
  }
};

//-----------------------------------------------------------------------------

checkbox::checkbox(gui *gui) : widget(gui), _is_checked(false), _is_mouse_over(false) {
}

checkbox::~checkbox() {
}

property *checkbox::text(std::string const &text) {
  return new checkbox_text_property(text);
}

void checkbox::on_attached_to_parent(widget *parent) {
  state_drawable *bkgnd = new state_drawable();
  bkgnd->add_drawable(state_drawable::normal, _gui->get_drawable_manager()->get_drawable("button_normal"));
  bkgnd->add_drawable(state_drawable::hover, _gui->get_drawable_manager()->get_drawable("button_hover"));
  _background = std::shared_ptr<drawable>(bkgnd);

  _check_icon = _gui->get_drawable_manager()->get_drawable("checkbox");
}

void checkbox::on_mouse_out() {
  _is_mouse_over = false;
  update_drawable_state();
}

void checkbox::on_mouse_over() {
  _is_mouse_over = true;
  update_drawable_state();
}

bool checkbox::on_mouse_down(float x, float y) {
  set_checked(!_is_checked);
  return widget::on_mouse_down(x, y);
}

void checkbox::set_checked(bool is_checked) {
  _is_checked = is_checked;
  update_drawable_state();
}

void checkbox::update_drawable_state() {
  state_drawable *drawable = dynamic_cast<state_drawable *>(_background.get());
  if (_is_mouse_over) {
    drawable->set_current_state(state_drawable::hover);
  } else {
    drawable->set_current_state(state_drawable::normal);
  }
}

void checkbox::render() {
  float left = get_left();
  float top = get_top();
  float width = get_width();
  float height = get_height();

  _background->render(left + 2, top + 2, height - 4, height - 4);

  if (_is_checked) {
    float icon_width = _check_icon->get_intrinsic_width();
    float icon_height = _check_icon->get_intrinsic_height();
    if (icon_width == 0.0f) {
      icon_width = height * 0.75f;
    }
    if (icon_height == 0.0f) {
      icon_height = height * 0.75f;
    }
    float x = left + (height / 2.0f) - (icon_width / 2.0f);
    float y = top + (height / 2.0f) - (icon_height / 2.0f);
    _check_icon->render(x, y, icon_width, icon_height);
  }

  if (_text.length() > 0) {
    fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
        left + height + 4, top + height / 2, _text,
        static_cast<fw::font_face::draw_flags>(fw::font_face::align_left | fw::font_face::align_middle));
  }
}

} }

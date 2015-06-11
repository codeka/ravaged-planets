
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/button.h>

#include <framework/framework.h>
#include <framework/font.h>


namespace fw { namespace gui {

/** Property that sets the background of the button. */
class button_background_property : public property {
private:
  std::string _drawable_name;
  std::shared_ptr<drawable> _drawable;
public:
  button_background_property(std::string const &drawable_name) :
      _drawable_name(drawable_name) {
  }
  button_background_property(std::shared_ptr<drawable> drawable) :
      _drawable(drawable) {
  }

  void apply(widget *widget) {
    button *btn = dynamic_cast<button *>(widget);
    if (_drawable) {
      btn->_background = _drawable;
    } else {
      btn->_background = btn->_gui->get_drawable_manager()->get_drawable(_drawable_name);
    }
  }
};

/** Property that sets the text of the button. */
class button_text_property : public property {
private:
  std::string _text;
public:
  button_text_property(std::string const &text) :
      _text(text) {
  }

  void apply(widget *widget) {
    button *btn = dynamic_cast<button *>(widget);
    btn->_text = _text;
  }
};

/** Property that sets the alignment of text on the button. */
class button_text_align_property : public property {
private:
  button::alignment _text_align;
public:
  button_text_align_property(button::alignment text_align) :
      _text_align(text_align) {
  }

  void apply(widget *widget) {
    button *btn = dynamic_cast<button *>(widget);
    btn->_text_align = _text_align;
  }
};

//-----------------------------------------------------------------------------

button::button(gui *gui) : widget(gui), _text_align(center) {
}

button::~button() {
}

property *button::background(std::string const &drawable_name) {
  return new button_background_property(drawable_name);
}

property *button::background(std::shared_ptr<drawable> drawable) {
  return new button_background_property(drawable);
}

property *button::text(std::string const &text) {
  return new button_text_property(text);
}

property *button::text_align(button::alignment align) {
  return new button_text_align_property(align);
}

void button::on_attached_to_parent(widget *parent) {
  // Assign default values for things that haven't been overwritten.
  if (!_background) {
    state_drawable *bkgnd = new state_drawable();
    bkgnd->add_drawable(state_drawable::normal, _gui->get_drawable_manager()->get_drawable("button_normal"));
    bkgnd->add_drawable(state_drawable::hover, _gui->get_drawable_manager()->get_drawable("button_hover"));
    _background = std::shared_ptr<drawable>(bkgnd);
  }
}

void button::on_mouse_out() {
  state_drawable *drawable = dynamic_cast<state_drawable *>(_background.get());
  if (drawable != nullptr) {
    drawable->set_current_state(state_drawable::normal);
  }
}

void button::on_mouse_over() {
  state_drawable *drawable = dynamic_cast<state_drawable *>(_background.get());
  if (drawable != nullptr) {
    drawable->set_current_state(state_drawable::hover);
  }
}

void button::render() {
  if (_background) {
    _background->render(get_left(), get_top(), get_width(), get_height());
  }

  if (_text.length() > 0) {
    if (_text_align == left) {
      fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
          get_left() + 4, get_top() + get_height() / 2, _text,
          static_cast<fw::font_face::draw_flags>(fw::font_face::align_left | fw::font_face::align_middle));
    } else if (_text_align == center) {
      fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
          get_left() + get_width() / 2, get_top() + get_height() / 2, _text,
          static_cast<fw::font_face::draw_flags>(fw::font_face::align_centre | fw::font_face::align_middle));
    }
  }
}

} }

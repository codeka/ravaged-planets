
#include <framework/audio.h>
#include <framework/framework.h>
#include <framework/font.h>
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/button.h>

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

/** Property that sets the icon of the button. */
class button_icon_property : public property {
private:
  std::string _drawable_name;
  std::shared_ptr<drawable> _drawable;
public:
  button_icon_property(std::string const &drawable_name) :
      _drawable_name(drawable_name) {
  }
  button_icon_property(std::shared_ptr<drawable> drawable) :
      _drawable(drawable) {
  }

  void apply(widget *widget) {
    button *btn = dynamic_cast<button *>(widget);
    if (_drawable) {
      btn->_icon = _drawable;
    } else {
      btn->_icon = btn->_gui->get_drawable_manager()->get_drawable(_drawable_name);
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

static std::shared_ptr<fw::audio_buffer> _hover_sound;

button::button(gui *gui) : widget(gui), _text_align(center), _is_pressed(false), _is_mouse_over(false) {
  sig_mouse_out.connect(std::bind(&button::on_mouse_out, this));
  sig_mouse_over.connect(std::bind(&button::on_mouse_over, this));

  if (!_hover_sound) {
    _hover_sound = fw::framework::get_instance()->get_audio_manager()->get_audio_buffer("gui/sounds/click.ogg");
  }
}

button::~button() {
}

property *button::background(std::string const &drawable_name) {
  return new button_background_property(drawable_name);
}

property *button::background(std::shared_ptr<drawable> drawable) {
  return new button_background_property(drawable);
}

property *button::icon(std::string const &drawable_name) {
  return new button_icon_property(drawable_name);
}

property *button::icon(std::shared_ptr<drawable> drawable) {
  return new button_icon_property(drawable);
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
    bkgnd->add_drawable(state_drawable::pressed, _gui->get_drawable_manager()->get_drawable("button_hover"));
    _background = std::shared_ptr<drawable>(bkgnd);
  }
}

void button::on_mouse_out() {
  _is_mouse_over = false;
  update_drawable_state();
}

void button::on_mouse_over() {
  _is_mouse_over = true;
  update_drawable_state();
}

void button::set_pressed(bool is_pressed) {
  _is_pressed = is_pressed;
  update_drawable_state();
}

void button::update_drawable_state() {
  state_drawable *drawable = dynamic_cast<state_drawable *>(_background.get());
  if (drawable == nullptr) {
    return;
  }

  if (!is_enabled()) {
    drawable->set_current_state(state_drawable::disabled);
  } else if (_is_pressed) {
    drawable->set_current_state(state_drawable::pressed);
  } else if (_is_mouse_over) {
    drawable->set_current_state(state_drawable::hover);
  } else {
    drawable->set_current_state(state_drawable::normal);
  }
}

void button::render() {
  float left = get_left();
  float top = get_top();
  float width = get_width();
  float height = get_height();

  if (_background) {
    _background->render(left, top, width, height);
  }

  if (_icon) {
    float icon_width = _icon->get_intrinsic_width();
    float icon_height = _icon->get_intrinsic_height();
    if (icon_width == 0.0f) {
      icon_width = width * 0.75f;
    }
    if (icon_height == 0.0f) {
      icon_height = height * 0.75f;
    }
    float x = left + (width / 2.0f) - (icon_width / 2.0f);
    float y = top + (height / 2.0f) - (icon_height / 2.0f);
    _icon->render(x, y, icon_width, icon_height);
  }

  if (_text.length() > 0) {
    if (_text_align == button::left) {
      fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
          left + 4, top + height / 2, _text,
          static_cast<fw::font_face::draw_flags>(fw::font_face::align_left | fw::font_face::align_middle));
    } else if (_text_align == button::center) {
      fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
          left + width / 2, top + height / 2, _text,
          static_cast<fw::font_face::draw_flags>(fw::font_face::align_centre | fw::font_face::align_middle));
    }
  }
}

} }


#include <framework/framework.h>
#include <framework/font.h>
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>

namespace fw { namespace gui {

/** Property that sets the background of the widget. */
class static_background_property : public property {
private:
  std::string _drawable_name;
public:
  static_background_property(std::string const &drawable_name) :
      _drawable_name(drawable_name) {
  }

  void apply(widget *widget) {
    label *wdgt = dynamic_cast<label *>(widget);
    wdgt->_background = wdgt->_gui->get_drawable_manager()->get_drawable(_drawable_name);
  }
};

/** Property that sets the text of the widget. */
class static_text_property : public property {
private:
  std::string _text;
public:
  static_text_property(std::string const &text) :
    _text(text) {
  }

  void apply(widget *widget) {
    label *wdgt = dynamic_cast<label *>(widget);
    wdgt->_text = _text;
  }
};

label::label(gui *gui) : widget(gui) {
}

label::~label() {
}

property *label::background(std::string const &drawable_name) {
  return new static_background_property(drawable_name);
}

property *label::text(std::string const &text) {
  return new static_text_property(text);
}

void label::render() {
  if (_background) {
    _background->render(get_left(), get_top(), get_width(), get_height());
  }
  if (_text != "") {
    fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
        get_left(), get_top() + get_height() / 2, _text,
        static_cast<fw::font_face::draw_flags>(fw::font_face::align_left | fw::font_face::align_middle));
  }
}

} }

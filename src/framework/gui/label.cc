
#include <framework/framework.h>
#include <framework/font.h>
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>

namespace fw { namespace gui {

/** Property that sets the background of the widget. */
class label_background_property : public property {
private:
  std::string _drawable_name;
  bool _centred;
public:
  label_background_property(std::string const &drawable_name, bool centred) :
      _drawable_name(drawable_name), _centred(centred) {
  }

  void apply(widget *widget) {
    label *wdgt = dynamic_cast<label *>(widget);
    wdgt->_background = wdgt->_gui->get_drawable_manager()->get_drawable(_drawable_name);
    wdgt->_background_centred = _centred;
  }
};

/** Property that sets the text of the widget. */
class label_text_property : public property {
private:
  std::string _text;
public:
  label_text_property(std::string const &text) :
    _text(text) {
  }

  void apply(widget *widget) {
    label *wdgt = dynamic_cast<label *>(widget);
    wdgt->_text = _text;
  }
};

/** Property that sets the text of the widget. */
class label_text_align_property : public property {
private:
  label::alignment _text_alignment;
public:
  label_text_align_property(label::alignment text_alignment) :
    _text_alignment(text_alignment) {
  }

  void apply(widget *widget) {
    label *wdgt = dynamic_cast<label *>(widget);
    wdgt->_text_alignment = _text_alignment;
  }
};

label::label(gui *gui) : widget(gui), _background_centred(false), _text_alignment(left) {
}

label::~label() {
}

property *label::background(std::string const &drawable_name, bool centred /*= false */) {
  return new label_background_property(drawable_name, centred);
}

property *label::text(std::string const &text) {
  return new label_text_property(text);
}

property *label::text_align(label::alignment text_alignment) {
  return new label_text_align_property(text_alignment);
}

void label::render() {
  if (_background) {
    if (_background_centred) {
      float background_width = _background->get_intrinsic_width();
      float background_height = _background->get_intrinsic_height();
      if (background_width == 0.0f) {
        background_width = get_width();
      }
      if (background_height == 0.0f) {
        background_height = get_height();
      }
      float x = get_left() + (get_width() / 2.0f) - (background_width / 2.0f);
      float y = get_top() + (get_height() / 2.0f) - (background_height / 2.0f);
      _background->render(x, y, background_width, background_height);
    } else {
      _background->render(get_left(), get_top(), get_width(), get_height());
    }
  }
  if (_text != "") {
    if (_text_alignment == label::left) {
      fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
          get_left(), get_top() + get_height() / 2, _text,
          static_cast<fw::font_face::draw_flags>(fw::font_face::align_left | fw::font_face::align_middle));
    } else if (_text_alignment == label::center) {
      fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
          get_left() + get_width() / 2, get_top() + get_height() / 2, _text,
          static_cast<fw::font_face::draw_flags>(fw::font_face::align_centre | fw::font_face::align_middle));
    } else if (_text_alignment == label::right) {
      fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
          get_left() + get_width(), get_top() + get_height() / 2, _text,
          static_cast<fw::font_face::draw_flags>(fw::font_face::align_right | fw::font_face::align_middle));
    }
  }
}

void label::set_text(std::string const &text) {
  _text = text;
}

void label::set_background(std::shared_ptr<drawable> background, bool centred /*= false */) {
  _background = background;
  _background_centred = centred;
}

} }

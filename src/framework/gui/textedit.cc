
#include <framework/framework.h>
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/textedit.h>
#include <framework/font.h>

#define STB_TEXTEDIT_CHARTYPE char
#define STB_TEXTEDIT_POSITIONTYPE int
#define STB_TEXTEDIT_UNDOSTATECOUNT 99
#define STB_TEXTEDIT_UNDOCHARCOUNT 999
#include <stb/stb_textedit.h>

namespace fw {
namespace gui {

/** Property that sets the text of the widget. */
class textedit_text_property : public property {
private:
  std::string _text;
public:
  textedit_text_property(std::string const &text) :
    _text(text) {
  }

  void apply(widget *widget) {
    textedit *te = dynamic_cast<textedit *>(widget);
    te->_text = _text;
  }
};

//-----------------------------------------------------------------------------

textedit::textedit(gui *gui) : widget(gui) {
  _background = gui->get_drawable_manager()->get_drawable("textedit");
}

textedit::~textedit() {
}

property *textedit::text(std::string const &text) {
  return new textedit_text_property(text);
}

void textedit::render() {
  _background->render(get_left(), get_top(), get_width(), get_height());

  if (_text != "") {
    fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
        get_left(), get_top() + get_height() / 2, _text,
        static_cast<fw::font_face::draw_flags>(fw::font_face::align_left | fw::font_face::align_middle),
        fw::colour::BLACK());
  }
}

}
}

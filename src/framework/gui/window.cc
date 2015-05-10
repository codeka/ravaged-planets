
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/window.h>

namespace fw { namespace gui {

/** Property that sets the background of the window. */
class window_background_property : public property {
private:
  std::string _drawable_name;
public:
  window_background_property(std::string const &drawable_name) :
      _drawable_name(drawable_name) {
  }

  void apply(widget *widget) {
    window *wnd = dynamic_cast<window *>(widget);
    wnd->_background = wnd->_gui->get_drawable_manager()->get_drawable(_drawable_name);
  }
};

//-----------------------------------------------------------------------------

window::window(gui *gui) : widget(gui) {
}

window::~window() {
}

void window::render() {
  if (_background) {
    _background->render(get_left(), get_top(), get_width(), get_height());
  }

  widget::render();
}

property *window::background(std::string const &drawable_name) {
  return new window_background_property(drawable_name);
}

} }

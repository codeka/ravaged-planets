
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/window.h>

namespace fw { namespace gui {

void background_property::apply(widget *widget) {
  window *wnd = dynamic_cast<window *>(widget);
  wnd->_background = wnd->_gui->get_drawable_manager()->get_drawable(_drawable_name);
}

//-----------------------------------------------------------------------------

window::window(gui *gui) :
  widget(gui) {
}

window::~window() {
}

void window::render() {
  if (_background) {
    _background->render(get_left(), get_top(), get_width(), get_height());
  }
}

} }

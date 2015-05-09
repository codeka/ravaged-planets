
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
  _gui(gui) {
}

window::~window() {
}

void window::render() {
  if (_background) {
    _background->render(_x.resolve(this), _y.resolve(this), _width.resolve(this), _height.resolve(this));
  }
}

} }

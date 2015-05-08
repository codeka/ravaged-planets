
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/window.h>

namespace fw { namespace gui {

window::window(gui *gui) :
  _gui(gui) {
}

window::~window() {
}

void window::set_background(std::string const &drawable_name) {
  _background = _gui->get_drawable_manager()->get_drawable(drawable_name);
}

void window::render() {
  if (_background) {
    _background->render();
  }
}

} }

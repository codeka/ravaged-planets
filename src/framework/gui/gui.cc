
#include <boost/foreach.hpp>

#include <framework/graphics.h>
#include <framework/paths.h>
#include <framework/gui/gui.h>
#include <framework/gui/drawable.h>
#include <framework/gui/window.h>

namespace fw { namespace gui {

gui::gui() :
  _graphics(nullptr), _drawable_manager(nullptr) {
}

gui::~gui() {
  if (_drawable_manager != nullptr) {
    delete _drawable_manager;
  }
}

void gui::initialize(fw::graphics *graphics) {
  _graphics = graphics;

  _drawable_manager = new drawable_manager();
  _drawable_manager->parse(fw::resolve("gui/drawables/drawables.xml"));
}

void gui::update(float dt) {
}

void gui::render() {
  BOOST_FOREACH(window *wnd, _top_level_windows) {
    wnd->render();
  }
}

window *gui::create_window() {
  window *wnd = new window(this);
  _top_level_windows.push_back(wnd);
  return wnd;
}

void gui::destroy_window(window *wnd) {
  _top_level_windows.erase(std::find(_top_level_windows.begin(), _top_level_windows.end(), wnd));
  delete wnd;
}

} }

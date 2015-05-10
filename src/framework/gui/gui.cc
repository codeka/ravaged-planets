
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
  BOOST_FOREACH(widget *widget, _top_level_widgets) {
    widget->render();
  }
}

void gui::attach_widget(widget *widget) {
  _top_level_widgets.push_back(widget);
}

void gui::detach_widget(widget *widget) {
  _top_level_widgets.erase(std::find(_top_level_widgets.begin(), _top_level_widgets.end(), widget));
  delete widget;
}

int gui::get_width() const {
  return _graphics->get_width();
}

int gui::get_height() const {
  return _graphics->get_height();
}

} }

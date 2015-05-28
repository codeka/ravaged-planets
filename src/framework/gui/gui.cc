
#include <boost/foreach.hpp>

#include <framework/framework.h>
#include <framework/input.h>
#include <framework/graphics.h>
#include <framework/paths.h>
#include <framework/gui/gui.h>
#include <framework/gui/drawable.h>
#include <framework/gui/window.h>

namespace fw { namespace gui {

gui::gui() :
  _graphics(nullptr), _drawable_manager(nullptr), _widget_under_mouse(nullptr), _widget_mouse_down(nullptr) {
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
  input *inp = fw::framework::get_instance()->get_input();
  widget *wdgt = get_widget_at(inp->mouse_x(), inp->mouse_y());
  if (wdgt != _widget_under_mouse) {
    if (_widget_under_mouse != nullptr) {
      _widget_under_mouse->on_mouse_out();
    }
    _widget_under_mouse = wdgt;
    if (_widget_under_mouse != nullptr) {
      _widget_under_mouse->on_mouse_over();
    }
  }
}

bool gui::inject_mouse(int button, bool is_down) {
  if (button != 1 || (is_down && _widget_under_mouse == nullptr)
      || (!is_down && _widget_mouse_down == nullptr)) {
    return false;
  }

  bool handled;
  if (is_down) {
    _widget_mouse_down = _widget_under_mouse;
    handled = _widget_mouse_down->on_mouse_down();
  } else {
    handled = _widget_mouse_down->on_mouse_up();
    _widget_mouse_down = nullptr;
  }
  return handled;
}

void gui::render() {
  BOOST_FOREACH(widget *widget, _top_level_widgets) {
    widget->render();
  }
}

widget *gui::get_widget_at(float x, float y) {
  BOOST_FOREACH(widget *wdgt, _top_level_widgets) {
    widget *child = wdgt->get_child_at(x, y);
    if (child != nullptr) {
      return child;
    }
  }

  return nullptr;
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

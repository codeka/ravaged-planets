
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
  _graphics(nullptr), _drawable_manager(nullptr), _widget_under_mouse(nullptr), _widget_mouse_down(nullptr),
  _focused(nullptr) {
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

  std::unique_lock<std::mutex> lock(_top_level_widget_mutex);
  BOOST_FOREACH(widget *wdgt, _pending_remove) {
    _top_level_widgets.erase(std::find(_top_level_widgets.begin(), _top_level_widgets.end(), wdgt));
    delete wdgt;
  }
  _pending_remove.clear();

  BOOST_FOREACH(widget *widget, _top_level_widgets) {
    if (widget->is_visible()) {
      widget->update(dt);
    }
  }
}

bool gui::inject_mouse(int button, bool is_down, float x, float y) {
  if (button != 1 || (is_down && _widget_under_mouse == nullptr)
      || (!is_down && _widget_mouse_down == nullptr)) {
    sig_click(button, is_down, nullptr);
    if (_focused != nullptr) {
      _focused->on_focus_lost();
      _focused = nullptr;
    }
    return false;
  }

  x -= _widget_under_mouse->get_left();
  y -= _widget_under_mouse->get_top();

  sig_click(button, is_down, _widget_under_mouse);
  if (is_down) {
    _widget_mouse_down = _widget_under_mouse;
    if (_widget_under_mouse->can_focus()) {
      if (_focused != nullptr) {
        _focused->on_focus_lost();
      }
      _focused = _widget_under_mouse;
      _focused->on_focus_gained();
    }
    propagate_mouse_event(_widget_mouse_down, true, x, y);
  } else {
    propagate_mouse_event(_widget_mouse_down, false, x, y);
    _widget_mouse_down = nullptr;
  }
  return true;
}

void gui::propagate_mouse_event(widget *w, bool is_down, float x, float y) {
  bool handled = (is_down ? w->on_mouse_down(x, y) : w->on_mouse_up(x, y));
  if (!handled && w->get_parent() != nullptr) {
    propagate_mouse_event(w->get_parent(), is_down, x, y);
  }
}

bool gui::inject_key(int key, bool is_down) {
  if (_focused != nullptr) {
    return _focused->on_key(key, is_down);
  }
  return false;
}

void gui::render() {
  FW_CHECKED(glEnable(GL_SCISSOR_TEST));
  std::unique_lock<std::mutex> lock(_top_level_widget_mutex);
  BOOST_FOREACH(widget *widget, _top_level_widgets) {
    if (widget->is_visible() && widget->prerender()) {
      widget->render();
      widget->postrender();
    }
  }
  FW_CHECKED(glDisable(GL_SCISSOR_TEST));
}

widget *gui::get_widget_at(float x, float y) {
  std::unique_lock<std::mutex> lock(_top_level_widget_mutex);
  // We want to make sure we pick the top-most widget at this position, so search in reverse.
  BOOST_REVERSE_FOREACH(widget *wdgt, _top_level_widgets) {
    if (!wdgt->is_visible()) {
      continue;
    }
    widget *child = wdgt->get_child_at(x, y);
    if (child != nullptr) {
      return child;
    }
  }

  return nullptr;
}

void gui::attach_widget(widget *widget) {
  std::unique_lock<std::mutex> lock(_top_level_widget_mutex);
  _top_level_widgets.push_back(widget);
}

void gui::detach_widget(widget *widget) {
  _pending_remove.push_back(widget);
}

void gui::bring_to_top(widget *widget) {
  std::unique_lock<std::mutex> lock(_top_level_widget_mutex);
  _top_level_widgets.erase(std::find(_top_level_widgets.begin(), _top_level_widgets.end(), widget));
  _top_level_widgets.push_back(widget);
}

int gui::get_width() const {
  return _graphics->get_width();
}

int gui::get_height() const {
  return _graphics->get_height();
}

} }

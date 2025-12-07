#include <framework/gui/gui.h>

#include <algorithm>

#include <framework/framework.h>
#include <framework/cursor.h>
#include <framework/input.h>
#include <framework/graphics.h>
#include <framework/paths.h>
#include <framework/signals.h>
#include <framework/gui/drawable.h>
#include <framework/gui/window.h>

namespace fw::gui {

Gui::Gui()
  : graphics_(nullptr) {
}

Gui::~Gui() {
}

fw::Status Gui::Initialize(fw::Graphics *graphics, fw::AudioManager* audio_manager) {
  graphics_ = graphics;

  RETURN_IF_ERROR(drawable_manager_.Parse(fw::resolve("gui/drawables/drawables.xml")));

  audio_source_ = audio_manager->create_audio_source();
  return fw::OkStatus();
}

void Gui::update(float dt) {
  Input *inp = fw::Framework::get_instance()->get_input();
  auto widget = get_widget_at(inp->mouse_x(), inp->mouse_y());
  auto widget_under_mouse = widget_under_mouse_.lock();
  if (widget != widget_under_mouse) {
    if (widget_under_mouse) {
      widget_under_mouse->sig_mouse_out.Emit();
    }
    widget_under_mouse = widget;
    if (widget_under_mouse) {
      widget_under_mouse->sig_mouse_over.Emit();
      fw::Framework::get_instance()->get_cursor()->set_cursor(
          2, widget_under_mouse->get_cursor_name());
    } else {
      fw::Framework::get_instance()->get_cursor()->set_cursor(2, "");
    }
    widget_under_mouse_ = widget_under_mouse;
  }
  if (widget_under_mouse && (inp->mouse_dx() != 0.0f || inp->mouse_dy() != 0.0f)) {
    float mx = inp->mouse_x() - widget_under_mouse->get_left();
    float my = inp->mouse_y() - widget_under_mouse->get_top();
    widget_under_mouse->sig_mouse_move.Emit(mx, my);
  }

  std::unique_lock<std::mutex> lock(top_level_widget_mutex_);
  for(auto widget : pending_remove_) {
    top_level_widgets_.erase(
        std::find(top_level_widgets_.begin(), top_level_widgets_.end(), widget));
  }
  pending_remove_.clear();

  for(auto widget : top_level_widgets_) {
    if (widget->is_visible()) {
      widget->update(dt);
    }
  }
}

bool Gui::inject_mouse(int button, bool is_down, float x, float y) {
  std::shared_ptr<Widget> widget_under_mouse = widget_under_mouse_.lock();
  std::shared_ptr<Widget> widget_mouse_down = widget_mouse_down_.lock();
  std::shared_ptr<Widget> focused = focused_.lock();
  if (button != 1 || (is_down && !widget_under_mouse)
      || (!is_down && !widget_mouse_down)) {
    sig_click_away.Emit(button, is_down);
    if (focused) {
      focused->on_focus_lost();
      focused_.reset();
    }
    return false;
  }

  if (!widget_under_mouse) {
    return false;
  }

  x -= widget_under_mouse->get_left();
  y -= widget_under_mouse->get_top();

  sig_click.Emit(button, is_down, *widget_under_mouse);
  if (is_down) {
    widget_mouse_down = widget_under_mouse;
    widget_mouse_down_ = widget_mouse_down;
    if (widget_under_mouse->can_focus()) {
      if (focused) {
        focused->on_focus_lost();
      }
      focused = widget_under_mouse;
      focused->on_focus_gained();
      focused_ = focused;
    }
    propagate_mouse_event(widget_mouse_down, true, x, y);
  } else {
    propagate_mouse_event(widget_mouse_down, false, x, y);
    widget_mouse_down_.reset();
  }
  return true;
}

void Gui::propagate_mouse_event(std::shared_ptr<Widget> w, bool is_down, float x, float y) {
  bool handled = (is_down ? w->on_mouse_down(x, y) : w->on_mouse_up(x, y));
  if (!handled && w->get_parent() != nullptr) {
    propagate_mouse_event(w->get_parent(), is_down, x, y);
  }
}

bool Gui::inject_key(int key, bool is_down) {
  std::shared_ptr<Widget> focused = focused_.lock();
  if (focused) {
    return focused->on_key(key, is_down);
  }
  return false;
}

void Gui::render() {
  glEnable(GL_SCISSOR_TEST);
  std::unique_lock<std::mutex> lock(top_level_widget_mutex_);
  for(auto widget : top_level_widgets_) {
    if (widget->is_visible() && widget->prerender()) {
      widget->render();
      widget->postrender();
    }
  }
  glDisable(GL_SCISSOR_TEST);
}

std::shared_ptr<Widget> Gui::get_widget_at(float x, float y) {
  std::unique_lock<std::mutex> lock(top_level_widget_mutex_);
  // We want to make sure we pick the top-most widget at this position, so search in reverse.
  for (auto it = top_level_widgets_.rbegin(); it != top_level_widgets_.rend(); ++it) {
    auto wdgt = *it;
    if (!wdgt->is_visible()) {
      continue;
    }
    auto child = wdgt->GetChildAt(x, y);
    if (child) {
      return child;
    }
  }

  return nullptr;
}

void Gui::attach_widget(std::shared_ptr<Widget> widget) {
  std::unique_lock<std::mutex> lock(top_level_widget_mutex_);
  top_level_widgets_.push_back(widget);
}

void Gui::detach_widget(std::shared_ptr<Widget> widget) {
  pending_remove_.push_back(widget);
}

bool Gui::IsAttached(Widget const &widget) {
  std::unique_lock<std::mutex> lock(top_level_widget_mutex_);
  for (auto top_level_widget : top_level_widgets_) {
    if (top_level_widget->IsChild(widget)) {
      return true;
    }
  }
  return false;
}

// If the given widget is attached to the view heriarchy, ensures that we're running on the render
// thread. Once attached to the heriarchy, all operations on widgets should be done on the
// render thread (you can perform operations in the view heriarchy while it's not attached).
void Gui::EnsureThread(Widget const &widget) {
  if (IsAttached(widget)) {
    fw::Graphics::ensure_render_thread();
  }
}

void Gui::bring_to_top(std::shared_ptr<Widget> widget) {
  std::unique_lock<std::mutex> lock(top_level_widget_mutex_);
  top_level_widgets_.erase(
      std::remove(top_level_widgets_.begin(), top_level_widgets_.end(), widget),
      top_level_widgets_.end());
  top_level_widgets_.push_back(widget);
}

int Gui::get_width() const {
  return graphics_->get_width();
}

int Gui::get_height() const {
  return graphics_->get_height();
}

}

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
  : graphics_(nullptr), drawable_manager_(nullptr), widget_under_mouse_(nullptr),
    widget_mouse_down_(nullptr), focused_(nullptr) {
}

Gui::~Gui() {
  if (drawable_manager_ != nullptr) {
    delete drawable_manager_;
  }
}

fw::Status Gui::Initialize(fw::Graphics *graphics, fw::AudioManager* audio_manager) {
  graphics_ = graphics;

  drawable_manager_ = new DrawableManager();
  RETURN_IF_ERROR(drawable_manager_->Parse(fw::resolve("gui/drawables/drawables.xml")));

  audio_source_ = audio_manager->create_audio_source();
  return fw::OkStatus();
}

void Gui::update(float dt) {
  Input *inp = fw::Framework::get_instance()->get_input();
  Widget *wdgt = get_widget_at(inp->mouse_x(), inp->mouse_y());
  if (wdgt != widget_under_mouse_) {
    if (widget_under_mouse_ != nullptr) {
      widget_under_mouse_->sig_mouse_out.Emit();
    }
    widget_under_mouse_ = wdgt;
    if (widget_under_mouse_ != nullptr) {
      widget_under_mouse_->sig_mouse_over.Emit();
      fw::Framework::get_instance()->get_cursor()->set_cursor(
          2, widget_under_mouse_->get_cursor_name());
    } else {
      fw::Framework::get_instance()->get_cursor()->set_cursor(2, "");
    }
  }
  if (widget_under_mouse_ != nullptr && (inp->mouse_dx() != 0.0f || inp->mouse_dy() != 0.0f)) {
    float mx = inp->mouse_x() - widget_under_mouse_->get_left();
    float my = inp->mouse_y() - widget_under_mouse_->get_top();
    widget_under_mouse_->sig_mouse_move.Emit(mx, my);
  }

  std::unique_lock<std::mutex> lock(top_level_widget_mutex_);
  for(Widget *wdgt : pending_remove_) {
    top_level_widgets_.erase(std::find(top_level_widgets_.begin(), top_level_widgets_.end(), wdgt));
    delete wdgt;
  }
  pending_remove_.clear();

  for(Widget *widget : top_level_widgets_) {
    if (widget->is_visible()) {
      widget->update(dt);
    }
  }
}

bool Gui::inject_mouse(int button, bool is_down, float x, float y) {
  if (button != 1 || (is_down && widget_under_mouse_ == nullptr)
      || (!is_down && widget_mouse_down_ == nullptr)) {
    sig_click.Emit(button, is_down, nullptr);
    if (focused_ != nullptr) {
      focused_->on_focus_lost();
      focused_ = nullptr;
    }
    return false;
  }

  if (widget_under_mouse_ == nullptr) {
    return false;
  }

  x -= widget_under_mouse_->get_left();
  y -= widget_under_mouse_->get_top();

  sig_click.Emit(button, is_down, widget_under_mouse_);
  if (is_down) {
    widget_mouse_down_ = widget_under_mouse_;
    if (widget_under_mouse_->can_focus()) {
      if (focused_ != nullptr) {
        focused_->on_focus_lost();
      }
      focused_ = widget_under_mouse_;
      focused_->on_focus_gained();
    }
    propagate_mouse_event(widget_mouse_down_, true, x, y);
  } else {
    propagate_mouse_event(widget_mouse_down_, false, x, y);
    widget_mouse_down_ = nullptr;
  }
  return true;
}

void Gui::propagate_mouse_event(Widget *w, bool is_down, float x, float y) {
  bool handled = (is_down ? w->on_mouse_down(x, y) : w->on_mouse_up(x, y));
  if (!handled && w->get_parent() != nullptr) {
    propagate_mouse_event(w->get_parent(), is_down, x, y);
  }
}

bool Gui::inject_key(int key, bool is_down) {
  if (focused_ != nullptr) {
    return focused_->on_key(key, is_down);
  }
  return false;
}

void Gui::render() {
  glEnable(GL_SCISSOR_TEST);
  std::unique_lock<std::mutex> lock(top_level_widget_mutex_);
  for(Widget *widget : top_level_widgets_) {
    if (widget->is_visible() && widget->prerender()) {
      widget->render();
      widget->postrender();
    }
  }
  glDisable(GL_SCISSOR_TEST);
}

Widget *Gui::get_widget_at(float x, float y) {
  std::unique_lock<std::mutex> lock(top_level_widget_mutex_);
  // We want to make sure we pick the top-most widget at this position, so search in reverse.
  for (auto it = top_level_widgets_.rbegin(); it != top_level_widgets_.rend(); ++it) {
    Widget* wdgt = *it;
    if (!wdgt->is_visible()) {
      continue;
    }
    Widget *child = wdgt->get_child_at(x, y);
    if (child != nullptr) {
      return child;
    }
  }

  return nullptr;
}

void Gui::attach_widget(Widget *widget) {
  std::unique_lock<std::mutex> lock(top_level_widget_mutex_);
  top_level_widgets_.push_back(widget);
}

void Gui::detach_widget(Widget *widget) {
  pending_remove_.push_back(widget);
}

void Gui::bring_to_top(Widget *widget) {
  std::unique_lock<std::mutex> lock(top_level_widget_mutex_);
  top_level_widgets_.erase(std::find(top_level_widgets_.begin(), top_level_widgets_.end(), widget));
  top_level_widgets_.push_back(widget);
}

int Gui::get_width() const {
  return graphics_->get_width();
}

int Gui::get_height() const {
  return graphics_->get_height();
}

}

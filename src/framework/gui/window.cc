
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/window.h>

namespace fw::gui {

class WindowInitialPositionProperty : public Property {
private:
  WindowInitialPosition initial_position_;
public:
  WindowInitialPositionProperty(WindowInitialPosition initial_position)
    : initial_position_(initial_position) {
  }

  void apply(Widget& widget) override {
    Window& wnd = dynamic_cast<Window&>(widget);
    wnd.initial_position_ = initial_position_;
  }
};

//-----------------------------------------------------------------------------

std::unique_ptr<Property> Window::initial_position(WindowInitialPosition initial_position) {
	return std::make_unique<WindowInitialPositionProperty>(initial_position);
}

Window::Window() : Widget() {
}

Window::~Window() {
}

void Window::render() {
  Widget::render();
}

void Window::update(float dt) {
  if (need_layout_) {
    need_layout_ = false;

		float screen_width = fw::Get<Gui>().get_width();
		float screen_height = fw::Get<Gui>().get_height();
    MeasuredSize measured_size =
        MeasureChild(
            MeasureSpec::Exactly(screen_width), 0.f,
            MeasureSpec::Exactly(screen_height), 0.f);
    PerformLayout(y_, x_ + measured_size.width, y_ + measured_size.height, x_);
  }
  if (need_initial_position_ && layout_params_) {
    need_initial_position_ = false;

    if (initial_position_.type == WindowInitialPosition::Type::kCenter) {
      x_ = (fw::Get<Gui>().get_width() / 2.f) - (get_width() / 2.f) + initial_position_.x;
      y_ = (fw::Get<Gui>().get_height() / 2.f) - (get_height() / 2.f) + initial_position_.y;
    }
    else if (initial_position_.type == WindowInitialPosition::Type::kAbsolute) {
      x_ = initial_position_.x + layout_params_->left_margin;
      y_ = initial_position_.y + layout_params_->top_margin;
    }
  }


  Widget::update(dt);
}

void Window::set_visible(bool visible) {
  Widget::set_visible(visible);

  if (visible) {
    // Also ask the framework to bring us to the top.
		std::shared_ptr<Window> shared_this =
        std::dynamic_pointer_cast<Window>(this->shared_from_this());
    fw::Get<Gui>().BringToTop(shared_this);
  }
}

void Window::OnAttachedToGui() {
  need_initial_position_ = true;
}

void Window::OnDetachedFromGui() {

}

void Window::RequestLayout() {
	need_layout_ = true;
}

}

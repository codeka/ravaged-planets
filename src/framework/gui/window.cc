
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/window.h>

namespace fw::gui {

/** Property that sets the background of the window. */
class WindowBackgroundProperty : public Property {
private:
  std::string drawable_name_;
public:
  WindowBackgroundProperty(std::string_view drawable_name)
      : drawable_name_(drawable_name) {
  }

  void apply(Widget &widget) override {
    Window &wnd = dynamic_cast<Window &>(widget);
    wnd.background_ = wnd.gui_->get_drawable_manager().get_drawable(drawable_name_);
  }
};

//-----------------------------------------------------------------------------

Window::Window(Gui *gui) : Widget(gui) {
}

Window::~Window() {
}

void Window::render() {
  if (background_) {
    background_->render(get_left(), get_top(), get_width(), get_height());
  }

  Widget::render();
}

std::unique_ptr<Property> Window::background(std::string_view drawable_name) {
  return std::make_unique<WindowBackgroundProperty>(drawable_name);
}

}

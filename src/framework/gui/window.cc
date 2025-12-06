
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/window.h>

namespace fw::gui {

/** Property that sets the background of the window. */
class WindowBackgroundProperty : public Property {
private:
  std::string _drawable_name;
public:
  WindowBackgroundProperty(std::string const &drawable_name) :
      _drawable_name(drawable_name) {
  }

  void apply(Widget *widget) {
    Window *wnd = dynamic_cast<Window *>(widget);
    wnd->background_ = wnd->gui_->get_drawable_manager().get_drawable(_drawable_name);
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

Property *Window::background(std::string const &drawable_name) {
  return new WindowBackgroundProperty(drawable_name);
}

}


#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/static_widget.h>

namespace fw { namespace gui {

/** Property that sets the background of the window. */
class static_background_property : public property {
private:
  std::string _drawable_name;
public:
  static_background_property(std::string const &drawable_name) :
      _drawable_name(drawable_name) {
  }

  void apply(widget *widget) {
    static_widget *wdgt = dynamic_cast<static_widget *>(widget);
    wdgt->_background = wdgt->_gui->get_drawable_manager()->get_drawable(_drawable_name);
  }
};

static_widget::static_widget(gui *gui) : widget(gui) {
}

static_widget::~static_widget() {
}

property *static_widget::background(std::string const &drawable_name) {
  return new static_background_property(drawable_name);
}

void static_widget::render() {
  if (_background) {
    _background->render(get_left(), get_top(), get_width(), get_height());
  }
}

} }

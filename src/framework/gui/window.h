#pragma once

#include <memory>

#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/widget.h>

namespace fw::gui {

// Represents a top-level window, complete with controls for moving and so on. All rendering happens inside a window.
class Window : public Widget {
private:
  friend class WindowBackgroundProperty;

  std::shared_ptr<Drawable> background_;

public:
  Window(Gui *gui);
  ~Window();

  static Property *background(std::string const &drawable_name);

  void render();
};

}

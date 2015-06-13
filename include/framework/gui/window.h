#pragma once

#include <memory>

#include <framework/gui/widget.h>

namespace fw { namespace gui {
class drawable;
class gui;


/**
 * Represents a top-level window, complete with controls for moving and so on. All rendering happens inside a window.
 */
class window : public widget {
private:
  friend class window_background_property;

  std::shared_ptr<drawable> _background;

public:
  window(gui *gui);
  ~window();

  static property *background(std::string const &drawable_name);

  void render();
};


} }

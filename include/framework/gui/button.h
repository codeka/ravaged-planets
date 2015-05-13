#pragma once

#include <memory>
#include <string>

#include <framework/gui/property.h>
#include <framework/gui/widget.h>

namespace fw { namespace gui {
class gui;
class drawable;

/** Buttons have a background image and text (or image, or both). */
class button : public widget {
private:
  friend class button_background_property;

  std::shared_ptr<drawable> _background;

public:
  button(gui *gui);
  virtual ~button();

  static property *background(std::string const &drawable_name);
  static property *background(std::shared_ptr<drawable> drawable);

  void on_attached_to_parent(widget *parent);
  void on_mouse_out();
  void on_mouse_over();
  void render();

};

} }

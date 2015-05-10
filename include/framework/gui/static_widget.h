#pragma once

#include <memory>
#include <string>

#include <framework/gui/property.h>
#include <framework/gui/widget.h>

namespace fw { namespace gui {
class gui;
class drawable;

/** A static widget is just a simple static control, either text or a drawable that is non-interactive. */
class static_widget : public widget {
private:
  friend class static_background_property;

  std::shared_ptr<drawable> _background;

public:
  static_widget(gui *gui);
  virtual ~static_widget();

  static property *background(std::string const &drawable_name);

  void render();
};

} }

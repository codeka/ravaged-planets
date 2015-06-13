#pragma once

#include <memory>
#include <string>

#include <framework/gui/widget.h>

namespace fw { namespace gui {
class gui;
class drawable;

/** A label is just a simple static control, either text or a drawable that is non-interactive. */
class label : public widget {
private:
  friend class static_background_property;
  friend class static_text_property;

  std::string _text;
  std::shared_ptr<drawable> _background;

public:
  label(gui *gui);
  virtual ~label();

  static property *background(std::string const &drawable_name);
  static property *text(std::string const &text);

  void render();
};

} }

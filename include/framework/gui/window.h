#pragma once

#include <memory>

#include <framework/gui/property.h>
#include <framework/gui/widget.h>

namespace fw { namespace gui {
class drawable;
class gui;

class background_property : public property {
private:
  std::string _drawable_name;
public:
  background_property(std::string const &drawable_name) :
      _drawable_name(drawable_name) {
  }

  void apply(widget *widget);
};

/**
 * Represents a top-level window. All rendering happens inside a window.
 */
class window : public widget {
private:
  friend class gui;
  friend class background_property;
  window(gui *gui);

  gui *_gui;
  std::shared_ptr<drawable> _background;

public:
  ~window();

  static inline property *background(std::string const &drawable_name) {
    return new background_property(drawable_name);
  }

  void render();
};


} }

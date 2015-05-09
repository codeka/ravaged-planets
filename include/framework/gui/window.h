
#include <memory>

#include <framework/gui/builder.h>

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

  void apply(window *wnd);
};

/**
 * Represents a top-level window. All rendering happens inside a window.
 */
class window {
private:
  friend class gui;
  friend class background_property;
  window(gui *gui);

  gui *_gui;
  std::shared_ptr<drawable> _background;

public:
  ~window();

  static inline property *background(std::string const &drawable_name) {
    return new fw::gui::background_property(drawable_name);
  }

  void render();
};


} }

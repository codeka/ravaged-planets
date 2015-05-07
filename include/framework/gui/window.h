
#include <memory>

namespace fw { namespace gui {
class drawable;
class gui;

/**
 * Represents a top-level window. All rendering happens inside a window.
 */
class window {
private:
  friend class gui;
  window(gui *gui);

  gui *_gui;
  std::shared_ptr<drawable> _background;

public:
  ~window();

  void set_background(std::string const &drawable_name);

  void render();
};

} }

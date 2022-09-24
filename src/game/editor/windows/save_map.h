#pragma once

#include <memory>

namespace fw {
namespace gui {
class Widget;
class Window;
}
class bitmap;
}

namespace ed {

class save_map_window {
private:
  fw::gui::Window *wnd_;

  bool save_clicked(fw::gui::Widget *w);
  bool cancel_clicked(fw::gui::Widget *w);
  bool screenshot_clicked(fw::gui::Widget *w);
  void screenshot_complete(std::shared_ptr<fw::Bitmap> bitmap);

  void update_screenshot();

public:
  save_map_window();
  virtual ~save_map_window();

  void initialize();
  void show();
  void hide();
};

extern save_map_window *save_map;

}

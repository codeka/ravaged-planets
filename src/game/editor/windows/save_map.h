#pragma once

#include <memory>

namespace fw {
namespace gui {
class widget;
class window;
}
class bitmap;
}

namespace ed {

class save_map_window {
private:
  fw::gui::window *_wnd;

  bool save_clicked(fw::gui::widget *w);
  bool cancel_clicked(fw::gui::widget *w);
  bool screenshot_clicked(fw::gui::widget *w);
  void screenshot_complete(std::shared_ptr<fw::bitmap> bitmap);

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

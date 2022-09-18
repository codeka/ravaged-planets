#pragma once

namespace fw {
namespace gui {
class widget;
class window;
}
}

namespace ed {

class open_map_window {
private:
  fw::gui::window *_wnd;

  bool open_clicked(fw::gui::widget *w);
  bool cancel_clicked(fw::gui::widget *w);

public:
  open_map_window();
  ~open_map_window();

  void initialize();
  void show();
  void hide();
};

extern open_map_window *open_map;

}

#pragma once

namespace fw {
namespace gui {
class window;
class widget;
}
}

namespace ed {

class new_map_window {
private:
  fw::gui::window *_wnd;

  bool ok_clicked(fw::gui::widget *w);
  bool cancel_clicked(fw::gui::widget *w);

public:
  new_map_window();
  ~new_map_window();

  void initialize();
  void show();
  void hide();
};

extern new_map_window *new_map;

}

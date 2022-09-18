#pragma once

namespace fw {
namespace gui {
class Window;
class Widget;
}
}

namespace ed {

class new_map_window {
private:
  fw::gui::Window *wnd_;

  bool ok_clicked(fw::gui::Widget *w);
  bool cancel_clicked(fw::gui::Widget *w);

public:
  new_map_window();
  ~new_map_window();

  void initialize();
  void show();
  void hide();
};

extern new_map_window *new_map;

}

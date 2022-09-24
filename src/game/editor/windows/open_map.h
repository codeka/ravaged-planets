#pragma once

namespace fw {
namespace gui {
class Widget;
class Window;
}
}

namespace ed {

class open_map_window {
private:
  fw::gui::Window *wnd_;

  bool open_clicked(fw::gui::Widget *w);
  bool cancel_clicked(fw::gui::Widget *w);

public:
  open_map_window();
  ~open_map_window();

  void initialize();
  void show();
  void hide();
};

extern open_map_window *open_map;

}

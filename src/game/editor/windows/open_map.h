#pragma once

namespace fw {
namespace gui {
class Widget;
class Window;
}
}

namespace ed {

class OpenMapWindow {
private:
  fw::gui::Window *wnd_;

  bool open_clicked(fw::gui::Widget *w);
  bool cancel_clicked(fw::gui::Widget *w);

public:
  OpenMapWindow();
  ~OpenMapWindow();

  void initialize();
  void show();
  void hide();
};

extern OpenMapWindow *open_map;

}

#pragma once

namespace fw {
namespace gui {
class Window;
class Widget;
}
}

namespace ed {

class NewMapWindow {
private:
  std::shared_ptr<fw::gui::Window> wnd_;

  bool ok_clicked(fw::gui::Widget &w);
  bool cancel_clicked(fw::gui::Widget &w);

public:
  NewMapWindow();
  ~NewMapWindow();

  void initialize();
  void show();
  void hide();
};

extern NewMapWindow *new_map;

}

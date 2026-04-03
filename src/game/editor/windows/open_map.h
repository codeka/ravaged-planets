#pragma once

#include <memory>

namespace fw {
namespace gui {
class Widget;
class Window;
}
}

namespace ed {

class OpenMapWindow {
private:
  std::shared_ptr<fw::gui::Window> wnd_;

  bool open_clicked(fw::gui::Widget &w);
  bool cancel_clicked(fw::gui::Widget &w);

public:
  OpenMapWindow();
  ~OpenMapWindow();

  void initialize();
  void show();
  void hide();
};

extern std::unique_ptr<OpenMapWindow> open_map;

}

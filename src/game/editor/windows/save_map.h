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

class SaveMapWindow {
private:
  std::shared_ptr<fw::gui::Window> wnd_;

  bool save_clicked(fw::gui::Widget &w);
  bool cancel_clicked(fw::gui::Widget &w);
  bool screenshot_clicked(fw::gui::Widget &w);
  void screenshot_complete(fw::Bitmap const &bitmap);

  void update_screenshot();

public:
  SaveMapWindow();
  virtual ~SaveMapWindow();

  void initialize();
  void show();
  void hide();
};

extern SaveMapWindow *save_map;

}

#pragma once

#include <memory>

#include <framework/gui/widget.h>
#include <framework/gui/window.h>

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

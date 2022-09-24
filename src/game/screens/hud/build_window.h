#pragma once

#include <memory>
#include <string>

namespace fw {
namespace gui {
class Window;
class Widget;
}
}

namespace ent {
class entity;
}
namespace game {

// The BuildWindow shows all of the units an entity with builder_component can build.
class BuildWindow {
private:
  fw::gui::Window *wnd_;
  std::weak_ptr<ent::entity> entity_;
  std::string build_group_;
  bool require_refresh_;
  int mouse_over_button_id_;

  void do_refresh();
  void on_mouse_over_button(int id);
  void on_mouse_out_button(int id);
  bool on_build_clicked(fw::gui::Widget *w, int id);
public:
  BuildWindow();
  virtual ~BuildWindow();

  void refresh(std::weak_ptr<ent::entity> entity, std::string build_group);

  void initialize();
  void update();
  void show();
  void hide();
};

extern BuildWindow *hud_build;

}

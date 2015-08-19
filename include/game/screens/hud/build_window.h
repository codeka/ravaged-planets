#pragma once

namespace fw {
namespace gui {
class window;
class widget;
}
}

namespace ent {
class entity;
}
namespace game {

// The build_window shows all of the units an entity with builder_component can build.
class build_window {
private:
  fw::gui::window *_wnd;
  std::weak_ptr<ent::entity> _entity;
  std::string _build_group;
  bool _require_refresh;

  void do_refresh();
public:
  build_window();
  virtual ~build_window();

  void refresh(std::weak_ptr<ent::entity> entity, std::string build_group);

  void initialize();
  void update();
  void show();
  void hide();
};

extern build_window *hud_build;

}

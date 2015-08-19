
#include <memory>
#include <boost/foreach.hpp>

#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>
#include <framework/gui/window.h>
#include <framework/logging.h>

#include <game/entities/entity.h>
#include <game/entities/entity_factory.h>
#include <game/screens/hud/build_window.h>

using namespace fw::gui;
using namespace std::placeholders;

namespace game {

build_window *hud_build = nullptr;

enum ids {
  _ID = 34636,
};

build_window::build_window() : _wnd(nullptr), _require_refresh(false) {
}

build_window::~build_window() {
  fw::framework::get_instance()->get_gui()->detach_widget(_wnd);
}

void build_window::initialize() {
  _wnd = builder<window>(sum(pct(100), px(-210)), px(220), px(200), px(200))
      << window::background("frame") << widget::visible(false)
      << (builder<button>(px(10), px(10), px(60), px(60)) << button::text("1"))
      << (builder<button>(px(70), px(10), px(60), px(60)) << button::text("2"))
      << (builder<button>(px(130), px(10), px(60), px(60)) << button::text("3"))
      << (builder<button>(px(10), px(70), px(60), px(60)) << button::text("4"))
      << (builder<button>(px(70), px(70), px(60), px(60)) << button::text("5"))
      << (builder<button>(px(130), px(70), px(60), px(60)) << button::text("6"))
      << (builder<button>(px(10), px(130), px(60), px(60)) << button::text("7"))
      << (builder<button>(px(70), px(130), px(60), px(60)) << button::text("8"))
      << (builder<button>(px(130), px(130), px(60), px(60)) << button::text("9"))
      ;
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);
}

void build_window::show() {
  _wnd->set_visible(true);
}

void build_window::hide() {
  _wnd->set_visible(false);
}

void build_window::refresh(std::weak_ptr<ent::entity> entity, std::string build_group) {
  _entity = entity;
  _build_group = build_group;
  _require_refresh = true;
}

void build_window::do_refresh() {
  std::vector<std::shared_ptr<ent::entity_template>> templates;
  ent::entity_factory ent_factory;
  ent_factory.get_buildable_templates(_build_group, templates);

  BOOST_FOREACH(std::shared_ptr<ent::entity_template> ent_template, templates) {

  }
}

void build_window::update() {
  if (_require_refresh) {
    do_refresh();
    _require_refresh = false;
  }
}

}


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
#include <game/screens/hud/build_window.h>

using namespace fw::gui;
using namespace std::placeholders;

namespace game {

build_window *hud_build = nullptr;

enum ids {
  _ID = 34636,
};

build_window::build_window() : _wnd(nullptr) {
}

build_window::~build_window() {
  fw::framework::get_instance()->get_gui()->detach_widget(_wnd);
}

void build_window::initialize() {
  _wnd = builder<window>(sum(pct(100), px(-210)), px(220), px(200), px(200))
      << window::background("frame") << widget::visible(false);
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);
}

void build_window::show() {
  _wnd->set_visible(true);
}

void build_window::hide() {
  _wnd->set_visible(false);
}

void build_window::refresh(std::weak_ptr<ent::entity> entity, std::string build_group) {
}

void build_window::update() {
}

}

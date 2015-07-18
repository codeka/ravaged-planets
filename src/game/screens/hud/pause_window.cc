
#include <functional>

#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/gui.h>
#include <framework/gui/window.h>

#include <game/screens/hud/pause_window.h>

using namespace fw::gui;
using namespace std::placeholders;

namespace game {

pause_window *hud_pause = nullptr;

pause_window::pause_window() : _wnd(nullptr) {
}

pause_window::~pause_window() {
  fw::framework::get_instance()->get_gui()->detach_widget(_wnd);
}

void pause_window::initialize() {
  _wnd = builder<window>(sum(pct(50), px(-75)), sum(pct(50), px(-100)), px(150), px(150))
      << window::background("frame") << widget::visible(false);
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);
}

void pause_window::show() {
  _wnd->set_visible(true);
}

void pause_window::hide() {
  _wnd->set_visible(false);
}

bool pause_window::is_visible() const {
  return _wnd->is_visible();
}

}

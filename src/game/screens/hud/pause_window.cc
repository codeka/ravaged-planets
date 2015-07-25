
#include <functional>

#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>
#include <framework/gui/window.h>
#include <framework/lang.h>

#include <game/application.h>
#include <game/screens/screen.h>
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
  _wnd = builder<window>(sum(pct(50), px(-75)), sum(pct(50), px(-100)), px(150), px(130))
      << window::background("frame") << widget::visible(false)
      << (builder<label>(px(4), px(4), sum(pct(100), px(-8)), px(20))
         << label::text(fw::text("hud.pause.title")))
      << (builder<button>(px(4), px(28), sum(pct(100), px(-8)), px(30))
          << button::text(fw::text("hud.pause.resume"))
          << button::click(std::bind(&pause_window::on_resume_clicked, this, _1)))
      << (builder<button>(px(4), px(62), sum(pct(100), px(-8)), px(30))
          << button::text(fw::text("hud.pause.exit-to-menu"))
          << button::click(std::bind(&pause_window::on_exit_to_menu_clicked, this, _1)))
      << (builder<button>(px(4), px(96), sum(pct(100), px(-8)), px(30))
          << button::text(fw::text("hud.pause.exit-game"))
          << button::click(std::bind(&pause_window::on_exit_game_clicked, this, _1)))
      ;
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);
}

bool pause_window::on_resume_clicked(widget *w) {
  fw::framework::get_instance()->unpause();
  hide();
  return true;
}

bool pause_window::on_exit_to_menu_clicked(widget *w) {
  fw::framework::get_instance()->unpause();

  application *app = dynamic_cast<application *>(fw::framework::get_instance()->get_app());
  screen_stack *ss = app->get_screen();
  ss->set_active_screen("title");

  return true;
}

bool pause_window::on_exit_game_clicked(widget *w) {
  fw::framework::get_instance()->unpause();
  fw::framework::get_instance()->exit();
  return true;
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

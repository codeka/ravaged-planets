#include <framework/framework.h>
#include <framework/misc.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/static_widget.h>
#include <framework/gui/window.h>
#include <framework/lang.h>
#include <framework/logging.h>
#include <framework/version.h>

#include <game/application.h>
#include <game/screens/screen.h>
#include <game/screens/title_screen.h>

namespace rp {

using namespace fw::gui;
using namespace std::placeholders;

// These are the instances of the various windows that are displayed by the title screen.
fw::gui::window *wnd;

title_screen::title_screen() {
}

title_screen::~title_screen() {
}

void title_screen::show() {
  fw::framework *frmwrk = fw::framework::get_instance();

  wnd = builder<window>(px(0), px(0), pct(100), pct(100)) << window::background("title_background")
      // Title "Ravaged Planets"
      << (builder<static_widget>(px(40), px(20), px(417), px(49)) << static_widget::background("title_heading"))
      // "A game by Dean Harding (dean@codeka.com.au)" text
      << (builder<static_widget>(px(40), px(70), px(500), px(16))
          << static_widget::text("A game by Dean Harding (dean@codeka.com.au)"))
      << (builder<button>(px(40), px(100), px(180), px(30)) << button::text("New Game")
          << widget::click(std::bind(&title_screen::newgame_clicked, this, _1)))
      << (builder<button>(px(40), px(140), px(180), px(30)) << button::text("Join Game"))
      << (builder<button>(px(40), px(180), px(180), px(30)) << button::text("Options"))
      << (builder<button>(px(40), px(220), px(180), px(30)) << button::text("Editor")
          << widget::click(std::bind(&title_screen::editor_clicked, this, _1)))
      << (builder<button>(px(40), px(260), px(180), px(30)) << button::text("Quit")
          << widget::click(std::bind(&title_screen::quit_clicked, this, _1)))
      // "v1.2.3"
      << (builder<static_widget>(sum(pct(50.0f), px(100)), sum(pct(100), px(-20)), px(500), px(16))
          << static_widget::text(fw::version_str));
  frmwrk->get_gui()->attach_widget(wnd);
}

bool title_screen::quit_clicked(fw::gui::widget *w) {
  fw::framework::get_instance()->exit();
  return true;
}

bool title_screen::newgame_clicked(fw::gui::widget *w) {
  rp::application *app = dynamic_cast<rp::application *>(fw::framework::get_instance()->get_app());
  app->get_screen()->set_active_screen("game");
  return true;
}

bool title_screen::editor_clicked(fw::gui::widget *w) {
  rp::application *app = dynamic_cast<rp::application *>(fw::framework::get_instance()->get_app());
  app->get_screen()->set_active_screen("editor");
  return true;
}

void title_screen::hide() {
  fw::framework *frmwrk = fw::framework::get_instance();
  frmwrk->get_gui()->detach_widget(wnd);
}

void title_screen::update() {
}

}

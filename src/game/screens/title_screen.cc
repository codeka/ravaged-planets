//#include "title/main_menu_window.h"
//#include "title/new_game_window.h"
//#include "title/join_game_window.h"
//#include "title/new_ai_player_window.h"
//#include "title/player_properties_window.h"
//#include "title/login_box_window.h"

#include <game/screens/title_screen.h>
#include <framework/framework.h>
#include <framework/misc.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/static_widget.h>
#include <framework/gui/window.h>
#include <framework/lang.h>

namespace rp {

// These are the instances of the various windows that are displayed by the title screen.
fw::gui::window *wnd;

title_screen::title_screen() {
}

title_screen::~title_screen() {
}

void title_screen::show() {
  fw::framework *frmwrk = fw::framework::get_instance();

  wnd = fw::gui::builder<fw::gui::window>()
      << fw::gui::widget::position(fw::gui::px(0), fw::gui::px(0))
      << fw::gui::widget::size(fw::gui::pct(100), fw::gui::pct(100))
      << fw::gui::window::background("title_background")
      << (fw::gui::builder<fw::gui::static_widget>()
          << fw::gui::widget::position(fw::gui::px(40), fw::gui::px(20))
          << fw::gui::widget::size(fw::gui::px(413), fw::gui::px(56))
          << fw::gui::static_widget::background("title_heading"))
      << (fw::gui::builder<fw::gui::static_widget>()
          << fw::gui::widget::position(fw::gui::px(40), fw::gui::px(100))
          << fw::gui::widget::size(fw::gui::px(180), fw::gui::px(30))
          << fw::gui::static_widget::background("button_normal"));
  frmwrk->get_gui()->attach_widget(wnd);

  /*
  frmwrk->get_gui()->set_active_layout("title");

  frmwrk->get_gui()->get_window("Title")->setVisible(true);
  CEGUI::Window *version = frmwrk->get_gui()->get_window("Title/VersionText");
  version->setText(fw::get_version_str().c_str());

  CEGUI::Window *subtitle = frmwrk->get_gui()->get_window("Title/SubtitleText");
  fw::gui::set_text(subtitle, (boost::format(fw::text("title.sub-title")) % "Dean Harding" % "dean@codeka.com").str());

  login_box->show(); // always visible
  main_menu->show();
*/
}

void title_screen::hide() {
/*
  fw::framework *frmwrk = fw::framework::get_instance();
  frmwrk->get_gui()->get_window("Title")->setVisible(false);
*/
}

void title_screen::update() {
}

}

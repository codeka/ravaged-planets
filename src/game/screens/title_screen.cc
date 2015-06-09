//#include "title/main_menu_window.h"
//#include "title/new_game_window.h"
//#include "title/join_game_window.h"
//#include "title/new_ai_player_window.h"
//#include "title/player_properties_window.h"
//#include "title/login_box_window.h"

#include <framework/framework.h>
#include <framework/misc.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/static_widget.h>
#include <framework/gui/window.h>
#include <framework/lang.h>
#include <framework/logging.h>

#include <game/application.h>
#include <game/screens/screen.h>
#include <game/screens/title_screen.h>

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
      // Title "Ravaged Planets"
      << (fw::gui::builder<fw::gui::static_widget>()
          << fw::gui::widget::position(fw::gui::px(40), fw::gui::px(20))
          << fw::gui::widget::size(fw::gui::px(417), fw::gui::px(49))
          << fw::gui::static_widget::background("title_heading"))
      // "A game by Dean Harding (dean@codeka.com.au)" text
      << (fw::gui::builder<fw::gui::static_widget>()
          << fw::gui::widget::position(fw::gui::px(40), fw::gui::px(70))
          << fw::gui::widget::size(fw::gui::px(500), fw::gui::px(16))
          << fw::gui::static_widget::text("A game by Dean Harding (dean@codeka.com.au)"))
      // "New Game" button
      << (fw::gui::builder<fw::gui::button>()
          << fw::gui::widget::position(fw::gui::px(40), fw::gui::px(100))
          << fw::gui::widget::size(fw::gui::px(180), fw::gui::px(30))
          << fw::gui::button::text("New Game"))
      // "Join Game" button
      << (fw::gui::builder<fw::gui::button>()
          << fw::gui::widget::position(fw::gui::px(40), fw::gui::px(140))
          << fw::gui::widget::size(fw::gui::px(180), fw::gui::px(30))
          << fw::gui::button::text("Join Game"))
      // "Options" button
      << (fw::gui::builder<fw::gui::button>()
          << fw::gui::widget::position(fw::gui::px(40), fw::gui::px(180))
          << fw::gui::widget::size(fw::gui::px(180), fw::gui::px(30))
          << fw::gui::button::text("Options"))
      // "Editor" button
      << (fw::gui::builder<fw::gui::button>()
          << fw::gui::widget::position(fw::gui::px(40), fw::gui::px(220))
          << fw::gui::widget::size(fw::gui::px(180), fw::gui::px(30))
          << fw::gui::button::text("Editor")
          << fw::gui::widget::click(std::bind(&title_screen::editor_clicked, this, std::placeholders::_1)))
      // "Quit" button
      << (fw::gui::builder<fw::gui::button>()
          << fw::gui::widget::position(fw::gui::px(40), fw::gui::px(260))
          << fw::gui::widget::size(fw::gui::px(180), fw::gui::px(30))
          << fw::gui::button::text("Quit")
          << fw::gui::widget::click(std::bind(&title_screen::quit_clicked, this, std::placeholders::_1)))
      // "v1.2.3"
      << (fw::gui::builder<fw::gui::static_widget>()
          << fw::gui::widget::position(fw::gui::px(40), fw::gui::px(300))
          << fw::gui::widget::size(fw::gui::px(500), fw::gui::px(16))
          << fw::gui::static_widget::text("v0.1 build 3 (debug)"));
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

bool title_screen::quit_clicked(fw::gui::widget *w) {
  fw::framework::get_instance()->exit();
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

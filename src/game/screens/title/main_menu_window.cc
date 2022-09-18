#include <functional>
#include <string>
#include <boost/format.hpp>

#include <framework/framework.h>
#include <framework/lang.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/widget.h>
#include <framework/gui/window.h>
#include <framework/gui/button.h>
#include <framework/gui/label.h>
#include <framework/version.h>

#include <game/application.h>
#include <game/screens/title/main_menu_window.h>
#include <game/screens/title/new_game_window.h>
#include <game/screens/title/join_game_window.h>
#include <game/screens/screen.h>
#include <game/session/session.h>

using namespace std::placeholders;
using namespace fw::gui;

namespace game {

main_menu_window::main_menu_window() : _exiting(false), _new_game_window(nullptr), _wnd(nullptr) {
}

main_menu_window::~main_menu_window() {
  if (_wnd != nullptr) {
    fw::framework::get_instance()->get_gui()->detach_widget(_wnd);
  }
}

void main_menu_window::initialize(new_game_window *new_game_window) {
  _new_game_window = new_game_window;
  _wnd = Builder<Window>(px(0), px(0), pct(100), pct(100)) << Window::background("title_background")
      << Widget::visible(false)
      // Title "Ravaged Planets"
      << (Builder<Label>(px(40), px(20), px(417), px(49)) << Label::background("title_heading"))
      // "A game by Dean Harding (dean@codeka.com.au)" text
      << (Builder<Label>(px(40), px(70), px(500), px(16))
          << Label::text((boost::format(fw::text("title.sub-title")) % "Dean Harding" % "dean@codeka.com.au").str()))
      << (Builder<Button>(px(40), px(100), px(180), px(30)) << Button::text(fw::text("title.new-game"))
          << Widget::click(std::bind(&main_menu_window::new_game_clicked, this, _1)))
      << (Builder<Button>(px(40), px(140), px(180), px(30)) << Button::text(fw::text("title.join-game")))
      << (Builder<Button>(px(40), px(180), px(180), px(30)) << Button::text(fw::text("title.options")))
      << (Builder<Button>(px(40), px(220), px(180), px(30)) << Button::text(fw::text("title.editor"))
          << Widget::click(std::bind(&main_menu_window::editor_clicked, this, _1)))
      << (Builder<Button>(px(40), px(260), px(180), px(30)) << Button::text(fw::text("title.quit"))
          << Widget::click(std::bind(&main_menu_window::quit_clicked, this, _1)))
      // "v1.2.3"
      << (Builder<Label>(sum(pct(50.0f), px(100)), sum(pct(100), px(-20)), px(500), px(16))
          << Label::text(fw::version_str));
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);
}

void main_menu_window::show() {
  _wnd->set_visible(true);
}

void main_menu_window::hide() {
  _wnd->set_visible(false);
}

void main_menu_window::update() {
  // if they've clicked "quit" and we're now logged out, then exit
  if (_exiting
      && (session::get_instance()->get_state() == session::disconnected
          || session::get_instance()->get_state() == session::in_error)) {
    fw::framework::get_instance()->exit();
  }
}

bool main_menu_window::new_game_clicked(Widget *w) {
  hide();
  _new_game_window->show();
  return true;
}

bool main_menu_window::join_game_clicked(Widget *w) {
  hide();
  //join_game->show();
  return true;
}

bool main_menu_window::editor_clicked(Widget *w) {
  application *app = dynamic_cast<application *>(fw::framework::get_instance()->get_app());
  screen_stack *ss = app->get_screen();

  ss->set_active_screen("editor");
  return true;
}

// This is called when you click "Quit", we need to exit...
bool main_menu_window::quit_clicked(Widget *w) {
  _exiting = true;

  if (session::get_instance()->get_state() != game::session::disconnected) {
    // if we're logged in, we want to log out before we quit
    session::get_instance()->logout();
    _exiting = true;

    // make sure they don't try to click "quit" more than once...
//    CEGUI::Window *quit_game_btn = get_child("MainMenu/QuitBtn");
//    quit_game_btn->setEnabled(false);

    // display a brief "please wait" message so they know what's happening
//    CEGUI::Window *overlay = get_parent()->getChild("PleaseWaitOverlay");
//    fw::gui::set_text(overlay->getChild("PleaseWaitOverlay/Message"), fw::text("please-wait.logging-out"));
//    overlay->setVisible(true);
//    overlay->moveToFront();
  } else {
    // if we're not logged in, just quit
    fw::framework::get_instance()->exit();
  }

  return true;
}
}

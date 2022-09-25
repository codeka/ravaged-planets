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

MainMenuWindow::MainMenuWindow() : exiting_(false), new_game_window_(nullptr), wnd_(nullptr) {
}

MainMenuWindow::~MainMenuWindow() {
  if (wnd_ != nullptr) {
    fw::Framework::get_instance()->get_gui()->detach_widget(wnd_);
  }
}

void MainMenuWindow::initialize(NewGameWindow * new_game_window) {
  new_game_window_ = new_game_window;
  wnd_ = Builder<Window>(px(0), px(0), pct(100), pct(100)) << Window::background("title_background")
      << Widget::visible(false)
      // Title "Ravaged Planets"
      << (Builder<Label>(px(40), px(20), px(417), px(49)) << Label::background("title_heading"))
      // "A game by Dean Harding (dean@codeka.com.au)" text
      << (Builder<Label>(px(40), px(70), px(500), px(16))
          << Label::text((boost::format(fw::text("title.sub-title")) % "Dean Harding" % "dean@codeka.com.au").str()))
      << (Builder<Button>(px(40), px(100), px(180), px(30)) << Button::text(fw::text("title.new-game"))
          << Widget::click(std::bind(&MainMenuWindow::new_game_clicked, this, _1)))
      << (Builder<Button>(px(40), px(140), px(180), px(30)) << Button::text(fw::text("title.join-game")))
      << (Builder<Button>(px(40), px(180), px(180), px(30)) << Button::text(fw::text("title.options")))
      << (Builder<Button>(px(40), px(220), px(180), px(30)) << Button::text(fw::text("title.editor"))
          << Widget::click(std::bind(&MainMenuWindow::editor_clicked, this, _1)))
      << (Builder<Button>(px(40), px(260), px(180), px(30)) << Button::text(fw::text("title.quit"))
          << Widget::click(std::bind(&MainMenuWindow::quit_clicked, this, _1)))
      // "v1.2.3"
      << (Builder<Label>(sum(pct(50.0f), px(100)), sum(pct(100), px(-20)), px(500), px(16))
          << Label::text(fw::version_str));
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);
}

void MainMenuWindow::show() {
  wnd_->set_visible(true);
}

void MainMenuWindow::hide() {
  wnd_->set_visible(false);
}

void MainMenuWindow::update() {
  // if they've clicked "quit" and we're now logged out, then exit
  if (exiting_
      && (Session::get_instance()->get_state() == Session::SessionState::kDisconnected
          || Session::get_instance()->get_state() == Session::SessionState::kInError)) {
    fw::Framework::get_instance()->exit();
  }
}

bool MainMenuWindow::new_game_clicked(Widget *w) {
  hide();
  new_game_window_->show();
  return true;
}

bool MainMenuWindow::join_game_clicked(Widget *w) {
  hide();
  //join_game->show();
  return true;
}

bool MainMenuWindow::editor_clicked(Widget *w) {
  Application *app = dynamic_cast<Application *>(fw::Framework::get_instance()->get_app());
  ScreenStack *ss = app->get_screen_stack();

  ss->set_active_screen("editor");
  return true;
}

// This is called when you click "Quit", we need to exit...
bool MainMenuWindow::quit_clicked(Widget *w) {
  exiting_ = true;

  if (Session::get_instance()->get_state() != Session::SessionState::kDisconnected) {
    // if we're logged in, we want to log out before we quit
    Session::get_instance()->logout();
    exiting_ = true;

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
    fw::Framework::get_instance()->exit();
  }

  return true;
}
}

#include <format>
#include <functional>
#include <string>

#include <framework/framework.h>
#include <framework/lang.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>
#include <framework/gui/linear_layout.h>
#include <framework/gui/widget.h>
#include <framework/gui/window.h>
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
    fw::Get<Gui>().DetachWindow(wnd_);
  }
}

void MainMenuWindow::initialize(NewGameWindow * new_game_window) {
  new_game_window_ = new_game_window;
  std::string name = "Dean Harding";
  std::string email = "dean@codeka.com";
  wnd_ = Builder<Window>()
      << Widget::width(LayoutParams::kMatchParent, 0.f)
      << Widget::height(LayoutParams::kMatchParent, 0.f)
      << Window::background("title_background")
      << Widget::visible(false)
      << (Builder<LinearLayout>()
          << Widget::width(LayoutParams::kMatchParent, 0.f)
          << Widget::height(LayoutParams::kWrapContent, 0.f)
          << Widget::margin(60.f, 0.f, 0.f, 40.f)
				  << LinearLayout::orientation(LinearLayout::Orientation::kVertical)
          // Title "Ravaged Planets"
          << (Builder<Label>()
				      //<< Widget::width(LayoutParams::kWrapContent, 0.f)
				      //<< Widget::height(LayoutParams::kWrapContent, 0.f)
              << Widget::width(LayoutParams::kFixed, 417.f)
              << Widget::height(LayoutParams::kFixed, 49.f)
              << Label::background("title_heading"))
          // "A game by Dean Harding (dean@codeka.com.au)" text
         << (Builder<Label>()
              //<< Widget::width(LayoutParams::kWrapContent, 0.f)
              //<< Widget::height(LayoutParams::kWrapContent, 0.f)
              << Widget::width(LayoutParams::kFixed, 500.f)
              << Widget::height(LayoutParams::kFixed, 16.f)
              << Label::text(std::vformat(fw::text("title.sub-title"), std::make_format_args(name, email))))
          << (Builder<Button>()
              << Button::text(fw::text("title.new-game"))
              << Widget::width(LayoutParams::kFixed, 180)
              << Widget::height(LayoutParams::kFixed, 30.f)
              << Widget::margin(120.f, 0.f, 0.f, 0.f)
              << Widget::click(std::bind(&MainMenuWindow::new_game_clicked, this, _1)))
          << (Builder<Button>()
              << Widget::width(LayoutParams::kFixed, 180)
              << Widget::height(LayoutParams::kFixed, 30.f)
              << Widget::margin(20.f, 0.f, 0.f, 0.f)
              << Button::text(fw::text("title.join-game")))
          << (Builder<Button>()
              << Widget::width(LayoutParams::kFixed, 180)
              << Widget::height(LayoutParams::kFixed, 30.f)
              << Widget::margin(20.f, 0.f, 0.f, 0.f)
              << Button::text(fw::text("title.options")))
          << (Builder<Button>()
              << Widget::width(LayoutParams::kFixed, 180)
              << Widget::height(LayoutParams::kFixed, 30.f)
              << Widget::margin(20.f, 0.f, 0.f, 0.f)
              << Button::text(fw::text("title.editor"))
              << Widget::click(std::bind(&MainMenuWindow::editor_clicked, this, _1)))
          << (Builder<Button>()
              << Widget::width(LayoutParams::kFixed, 180)
              << Widget::height(LayoutParams::kFixed, 30.f)
              << Widget::margin(20.f, 0.f, 0.f, 0.f)
              << Button::text(fw::text("title.quit"))
              << Widget::click(std::bind(&MainMenuWindow::quit_clicked, this, _1))))
      // "v1.2.3"
      << (Builder<Label>()
          //<< Widget::width(LayoutParams::kWrapContent, 0.f)
          //<< Widget::height(LayoutParams::kWrapContent, 0.f)
          << Widget::width(LayoutParams::kFixed, 500.f)
          << Widget::height(LayoutParams::kFixed, 16.f)
				  << Widget::margin(0.f, 0.f, 20.f, 0.f) // TODO: align bottom?
          << Label::text(fw::version_str));
  fw::Get<Gui>().AttachWindow(wnd_);
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

bool MainMenuWindow::new_game_clicked(Widget &w) {
  hide();
  new_game_window_->show();
  return true;
}

bool MainMenuWindow::join_game_clicked(Widget &w) {
  hide();
  //join_game->show();
  return true;
}

bool MainMenuWindow::editor_clicked(Widget &w) {
  Application *app = dynamic_cast<Application *>(fw::Framework::get_instance()->get_app());
  ScreenStack *ss = app->get_screen_stack();

  ss->set_active_screen("editor");
  return true;
}

// This is called when you click "Quit", we need to exit...
bool MainMenuWindow::quit_clicked(Widget &w) {
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

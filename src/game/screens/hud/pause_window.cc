
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

PauseWindow *hud_pause = nullptr;

PauseWindow::PauseWindow() : wnd_(nullptr) {
}

PauseWindow::~PauseWindow() {
  fw::Framework::get_instance()->get_gui()->detach_widget(wnd_);
}

void PauseWindow::initialize() {
  wnd_ = Builder<Window>(sum(pct(50), px(-75)), sum(pct(50), px(-100)), px(150), px(130))
      << Window::background("frame") << Widget::visible(false)
      << (Builder<Label>(px(4), px(4), sum(pct(100), px(-8)), px(20))
         << Label::text(fw::text("hud.pause.title")))
      << (Builder<Button>(px(4), px(28), sum(pct(100), px(-8)), px(30))
          << Button::text(fw::text("hud.pause.resume"))
          << Button::click(std::bind(&PauseWindow::on_resume_clicked, this, _1)))
      << (Builder<Button>(px(4), px(62), sum(pct(100), px(-8)), px(30))
          << Button::text(fw::text("hud.pause.exit-to-menu"))
          << Button::click(std::bind(&PauseWindow::on_exit_to_menu_clicked, this, _1)))
      << (Builder<Button>(px(4), px(96), sum(pct(100), px(-8)), px(30))
          << Button::text(fw::text("hud.pause.exit-game"))
          << Button::click(std::bind(&PauseWindow::on_exit_game_clicked, this, _1)))
      ;
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);
}

bool PauseWindow::on_resume_clicked(Widget &w) {
  fw::Framework::get_instance()->unpause();
  hide();
  return true;
}

bool PauseWindow::on_exit_to_menu_clicked(Widget &w) {
  fw::Framework::get_instance()->unpause();

  Application *app = dynamic_cast<Application *>(fw::Framework::get_instance()->get_app());
  ScreenStack *ss = app->get_screen_stack();
  ss->set_active_screen("title");

  return true;
}

bool PauseWindow::on_exit_game_clicked(Widget &w) {
  fw::Framework::get_instance()->unpause();
  fw::Framework::get_instance()->exit();
  return true;
}


void PauseWindow::show() {
  wnd_->set_visible(true);
}

void PauseWindow::hide() {
  wnd_->set_visible(false);
}

bool PauseWindow::is_visible() const {
  return wnd_->is_visible();
}

}

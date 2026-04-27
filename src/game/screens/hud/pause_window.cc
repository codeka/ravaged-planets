
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
#include <framework/gui/linear_layout.h>

using namespace fw::gui;
using namespace std::placeholders;

namespace game {

PauseWindow *hud_pause = nullptr;

PauseWindow::PauseWindow() : wnd_(nullptr) {
}

PauseWindow::~PauseWindow() {
  fw::Get<Gui>().DetachWindow(wnd_);
}

void PauseWindow::initialize() {
  wnd_ = Builder<Window>()
      << Widget::width(Widget::Fixed(300.f))
      << Widget::height(Widget::WrapContent())
      << Window::initial_position(WindowInitialPosition::Center())
      << Widget::background("frame")
      << Widget::visible(false)
      << (Builder<LinearLayout>()
          << Widget::width(Widget::MatchParent())
          << Widget::height(Widget::WrapContent())
          << LinearLayout::orientation(LinearLayout::Orientation::kVertical)
          << (Builder<Label>()
              << Widget::width(Widget::MatchParent())
              << Widget::height(Widget::WrapContent())
              << Label::text(fw::text("hud.pause.title")))
          << (Builder<Button>()
              << Widget::width(Widget::MatchParent())
              << Widget::height(Widget::Fixed(30.f))
              << Button::text(fw::text("hud.pause.resume"))
              << Button::click(std::bind(&PauseWindow::on_resume_clicked, this, _1)))
          << (Builder<Button>()
              << Widget::width(Widget::MatchParent())
              << Widget::height(Widget::Fixed(30.f))
              << Button::text(fw::text("hud.pause.exit-to-menu"))
              << Button::click(std::bind(&PauseWindow::on_exit_to_menu_clicked, this, _1)))
          << (Builder<Button>()
              << Widget::width(Widget::MatchParent())
              << Widget::height(Widget::Fixed(30.f))
              << Button::text(fw::text("hud.pause.exit-game"))
              << Button::click(std::bind(&PauseWindow::on_exit_game_clicked, this, _1)))
        )
      ;
  fw::Get<Gui>().AttachWindow(wnd_);
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

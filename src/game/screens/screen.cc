
#include <framework/exception.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <game/screens/screen.h>
#include <game/screens/title_screen.h>
#include <game/screens/game_screen.h>
#include <game/editor/editor_screen.h>

namespace game {

Screen::Screen() {
}

Screen::~Screen() {
}

void Screen::show() {
}

void Screen::hide() {
}

void Screen::update() {
}

//-------------------------------------------------------------------------

ScreenStack::ScreenStack() {
  screens_["title"] = new TitleScreen();
  screens_["game"] = new GameScreen();
  screens_["editor"] = new ed::EditorScreen();
}

ScreenStack::~ScreenStack() {
  for (auto Screen : screens_) {
    delete Screen.second;
  }
}

void ScreenStack::set_active_screen(std::string const &name,
    std::shared_ptr<ScreenOptions> options /*= std::shared_ptr<ScreenOptions>()*/) {
  fw::Framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
    auto it = screens_.find(name);
    if (it == screens_.end()) {
      BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("invalid screen name!"));
    }

    if (active_ != name) {
      if (active_ != "") {
        std::string old_active = active_;
        active_ = "";
        screens_[old_active]->hide();
      }
      screens_[name]->set_options(options);
      screens_[name]->show();
      active_ = name;
    }
  });
}

Screen *ScreenStack::get_active_screen() {
  if (active_ == "") {
    return nullptr;
  }

  return screens_[active_];
}

}

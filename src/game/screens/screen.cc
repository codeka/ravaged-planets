
#include <framework/exception.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <game/screens/screen.h>
#include <game/screens/title_screen.h>
#include <game/screens/game_screen.h>
#include <game/editor/editor_screen.h>

namespace game {

screen::screen() {
}

screen::~screen() {
}

void screen::show() {
}

void screen::hide() {
}

void screen::update() {
}

void screen::render(fw::sg::Scenegraph &) {
}

//-------------------------------------------------------------------------

screen_stack::screen_stack() {
  _screens["title"] = new title_screen();
  _screens["game"] = new game_screen();
  _screens["editor"] = new ed::editor_screen();
}

screen_stack::~screen_stack() {
  for (auto screen : _screens) {
    delete screen.second;
  }
}

void screen_stack::set_active_screen(std::string const &name,
    std::shared_ptr<screen_options> options /*= std::shared_ptr<screen_options>()*/) {
  fw::Framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
    auto it = _screens.find(name);
    if (it == _screens.end()) {
      BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("invalid screen name!"));
    }

    if (active_ != name) {
      if (active_ != "") {
        std::string old_active = active_;
        active_ = "";
        _screens[old_active]->hide();
      }
      _screens[name]->set_options(options);
      _screens[name]->show();
      active_ = name;
    }
  });
}

screen *screen_stack::get_active_screen() {
  if (active_ == "") {
    return nullptr;
  }

  return _screens[active_];
}

}

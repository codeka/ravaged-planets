
#include <boost/foreach.hpp>

#include <game/screens/screen.h>
#include <game/screens/title_screen.h>
#include <game/editor/editor_screen.h>
#include <framework/exception.h>

namespace rp {

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

void screen::render(fw::sg::scenegraph &) {
}

//-------------------------------------------------------------------------

screen_stack::screen_stack() {
  _screens["title"] = new title_screen();
//  _screens.push_back(new game_screen());
  _screens["editor"] = new ed::editor_screen();
}

screen_stack::~screen_stack() {
  BOOST_FOREACH(auto screen, _screens) {
    delete screen.second;
  }
}

void screen_stack::set_active_screen(std::string const &name,
    std::shared_ptr<screen_options> options /*= std::shared_ptr<screen_options>()*/) {
  auto it = _screens.find(name);
  if (it == _screens.end()) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("invalid screen name!"));
  }

  if (_active != name) {
    if (_active != "") {
      _screens[_active]->hide();
    }

    _active = name;
    _screens[name]->set_options(options);
    _screens[name]->show();
  }
}

screen *screen_stack::get_active_screen() {
  if (_active == "") {
    return nullptr;
  }

  return _screens[_active];
}

}


#include <boost/foreach.hpp>

#include <game/screens/screen.h>
#include <game/screens/title_screen.h>
//#include "../../editor/editor_screen.h"
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

screen_stack::screen_stack() :
    _active(-1) {
  _screens.push_back(new title_screen());
//  _screens.push_back(new game_screen());
//  _screens.push_back(new ed::editor_screen());
}

screen_stack::~screen_stack() {
  BOOST_FOREACH(auto screen, _screens) {
    delete screen;
  }
}

void screen_stack::set_active_screen(std::string const &name,
    std::shared_ptr<screen_options> options /*= std::shared_ptr<screen_options>()*/) {
  // todo: is this the best way? I don't think so...
  int index;
  if (name == "title")
    index = 0;
  else if (name == "game")
    index = 1;
  else if (name == "editor")
    index = 2;
  else {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("invalid screen name!"));
    index = -1; // never gets here
  }

  if (_active != index) {
    if (_active >= 0 && _active < static_cast<int>(_screens.size()))
      _screens[_active]->hide();

    _active = index;
    _screens[index]->set_options(options);
    _screens[index]->show();
  }
}

screen *screen_stack::get_active_screen() {
  return _screens[_active];
}

}

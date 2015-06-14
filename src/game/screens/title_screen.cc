#include <framework/framework.h>
#include <framework/misc.h>
#include <framework/logging.h>

#include <game/application.h>
#include <game/screens/screen.h>
#include <game/screens/game_screen.h>
#include <game/screens/title_screen.h>
#include <game/screens/title/main_menu_window.h>
#include <game/screens/title/new_game_window.h>

namespace game {

using namespace fw::gui;
using namespace std::placeholders;

// These are the instances of the various windows that are displayed by the title screen.
fw::gui::window *wnd;

title_screen::title_screen() : _main_menu_window(nullptr), _new_game_window(nullptr) {
}

title_screen::~title_screen() {
}

void title_screen::show() {
  _main_menu_window = new main_menu_window();
  _new_game_window = new new_game_window();
  _main_menu_window->initialize(_new_game_window);
  _new_game_window->initialize(_main_menu_window);

  _main_menu_window->show();
}

void title_screen::hide() {
  delete _main_menu_window;
  _main_menu_window = nullptr;
  delete _new_game_window;
  _new_game_window = nullptr;
}

void title_screen::update() {
  _main_menu_window->update();
}

}

#include <iostream>

#include <boost/exception/all.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>

#include <framework/settings.h>
#include <framework/logging.h>
#include <framework/misc.h>

#include <game/application.h>

namespace game {
  void settings_initialize(int argc, char** argv);
}

void display_exception(std::string const &msg);

extern "C" {

int main(int argc, char** argv) {
  try {
    game::settings_initialize(argc, argv);

    game::Application app;
    new fw::Framework(&app);
    fw::Framework::get_instance()->initialize("Ravaged Planet");

    fw::Framework::get_instance()->run();
  } catch(std::exception &e) {
    std::string msg = boost::diagnostic_information(e);
    fw::debug << "--------------------------------------------------------------------------------" << std::endl;
    fw::debug << "UNHANDLED EXCEPTION!" << std::endl;
    fw::debug << msg << std::endl;

    display_exception(e.what());
  } catch (...) {
    fw::debug << "--------------------------------------------------------------------------------" << std::endl;
    fw::debug << "UNHANDLED EXCEPTION! (unknown exception)" << std::endl;
  }

  return 0;
}

}

void display_exception(std::string const &msg) {
  std::stringstream ss;
  ss << "An error has occurred. Please send your log file (below) to dean@codeka.com.au for diagnostics." << std::endl;
  ss << std::endl;
  ss << fw::debug.get_filename() << std::endl;
  ss << std::endl;
  ss << msg;
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", ss.str().c_str(), nullptr);
}

#include <iostream>

#include <boost/exception/all.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>

#include <framework/settings.h>
#include <framework/logging.h>
#include <framework/misc.h>

#include <game/application.h>
#include <game/settings.h>

void display_exception(std::string const &msg);

extern "C" {

int main(int argc, char** argv) {
  try {
    auto status = game::settings_initialize(argc, argv);
    if (!status.ok()) {
      std::stringstream ss;
      ss << status << std::endl;
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", ss.str().c_str(), nullptr);
      return 1;
    }

    game::Application app;
    new fw::Framework(&app);
    auto continue_or_status = fw::Framework::get_instance()->initialize("Ravaged Planet");
    if (!continue_or_status.ok()) {
      LOG(ERR) << continue_or_status.status();
      return 1;
    }
    if (!continue_or_status.value()) {
      return 0;
    }

    fw::Framework::get_instance()->run();
  } catch(std::exception &e) {
    LOG(ERR) << "--------------------------------------------------------------------------------";
    LOG(ERR) << "UNHANDLED EXCEPTION!";
    LOG(ERR) << e.what();

    display_exception(e.what());
  } catch (...) {
    LOG(ERR) << "--------------------------------------------------------------------------------";
    LOG(ERR) << "UNHANDLED EXCEPTION! (unknown exception)";
  }

  return 0;
}

}

void display_exception(std::string const &msg) {
  std::stringstream ss;
  ss << "An error has occurred. Please send your log file (below) to dean@codeka.com.au for diagnostics." << std::endl;
  ss << std::endl;
  ss << fw::LogFileName() << std::endl;
  ss << std::endl;
  ss << msg;
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", ss.str().c_str(), nullptr);
}

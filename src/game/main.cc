#include <iostream>

#include <boost/exception/all.hpp>

#include <SDL.h>

#include "framework/settings.h"
#include "framework/logging.h"
#include "framework/misc.h"

#include "game/application.h"

namespace rp {
  void settings_initialize(int argc, char** argv);
}

void display_exception(std::string const &msg);

int main(int argc, char** argv) {
  try {
    rp::settings_initialize(argc, argv);

    rp::application app;
    new fw::framework(&app);
    fw::framework::get_instance()->initialize("Ravaged Planet");

    fw::debug << "Hello World!" << std::endl;

    fw::framework::get_instance()->run();
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

void display_exception(std::string const &msg) {
  std::stringstream ss;
  ss << "An error has occurred. Please send your log file (below) to dean@codeka.com.au for diagnostics." << std::endl;
  ss << std::endl;
  ss << fw::debug.get_filename() << std::endl;
  ss << std::endl;
  ss << msg;
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", ss.str().c_str(), nullptr);
}

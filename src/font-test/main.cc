#include <iostream>

#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>

#include <framework/bitmap.h>
#include <framework/settings.h>
#include <framework/framework.h>
#include <framework/font.h>
#include <framework/logging.h>
#include <framework/misc.h>
#include <framework/paths.h>

namespace po = boost::program_options;

void settings_initialize(int argc, char** argv);
void display_exception(std::string const &msg);

//-----------------------------------------------------------------------------

int main(int argc, char** argv) {
  try {
    settings_initialize(argc, argv);

    fw::tool_application app;
    new fw::framework(&app);
    fw::framework::get_instance()->initialize("Font Test");

    std::shared_ptr<fw::font_face> font_face = fw::framework::get_instance()->get_font_manager()->get_face();
    font_face->ensure_glyphs("ABCabcDEFdefGHIghiJKLjklMNOmnoPQRpqrSTUstuVWXvwxYZyz");
    font_face->get_bitmap()->save_bitmap(fw::resolve("test.png", true));
    fw::debug << "Bitmap saved to:" << fw::resolve("test.png", true) << std::endl;

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
}

void settings_initialize(int argc, char** argv) {
  po::options_description options("Additional options");
  options.add_options()
      ("font-file", po::value<std::string>()->default_value("gui/SaccoVanzetti.ttf"), "Name of the font to load, we assume it can be fw::resolve'd.")
    ;

  fw::settings::initialize(options, argc, argv, "font-test.conf");
}

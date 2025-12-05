#include <iostream>

#include <framework/bitmap.h>
#include <framework/logging.h>
#include <framework/settings.h>
#include <framework/framework.h>
#include <framework/font.h>
#include <framework/logging.h>
#include <framework/misc.h>
#include <framework/paths.h>
#include <framework/status.h>

fw::Status settings_initialize(int argc, char** argv);
void display_exception(std::string const &msg);

//-----------------------------------------------------------------------------

int main(int argc, char** argv) {
  try {
    auto status = settings_initialize(argc, argv);
    if (!status.ok()) {
      std::cerr << status << std::endl;
      fw::Settings::print_help();
      return 1;
    }

    fw::ToolApplication app;
    new fw::Framework(&app);
    auto continue_or_status = fw::Framework::get_instance()->initialize("Font Test");
    if (!continue_or_status.ok()) {
      LOG(ERR) << continue_or_status.status();
      return 1;
    }
    if (!continue_or_status.value()) {
      return 0;
    }

    auto font_face = fw::Framework::get_instance()->get_font_manager()->get_face();
    font_face->ensure_glyphs("wm");
    status = font_face->get_bitmap()->save_bitmap(fw::resolve("test.png", true));
    if (!status.ok()) {
      LOG(ERR) << status;
      return 1;
    }
    LOG(INFO) << "Bitmap saved to:" << fw::resolve("test.png", true);

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

void display_exception(std::string const &msg) {
  std::stringstream ss;
  ss << "An error has occurred. Please send your log file (below) to dean@codeka.com.au for diagnostics." << std::endl;
  ss << std::endl;
  ss << fw::LogFileName() << std::endl;
  ss << std::endl;
  ss << msg;
}

fw::Status settings_initialize(int argc, char** argv) {
  fw::SettingDefinition extra_settings;
  extra_settings.add_group("Additional options", "Font-test specific settings")
      .add_setting<std::string>(
          "font-file",
          "Name of the font to load, we assume it can be fw::resolve'd.",
          "gui/SaccoVanzetti.ttf");

  return fw::Settings::initialize(extra_settings, argc, argv, "font-test.conf");
}

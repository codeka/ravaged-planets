#include <iostream>

#include <boost/program_options.hpp>

#include <framework/bitmap.h>
#include <framework/camera.h>
#include <framework/settings.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/font.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/window.h>
#include <framework/logging.h>
#include <framework/particle_manager.h>
#include <framework/particle_effect.h>
#include <framework/misc.h>
#include <framework/paths.h>
#include <framework/scenegraph.h>

namespace po = boost::program_options;

void settings_initialize(int argc, char** argv);
void display_exception(std::string const &msg);

class application: public fw::base_app {
public:
  bool initialize(fw::framework *frmwrk);
  void update(float dt);
  void render(fw::sg::scenegraph &scenegraph);
};

bool click_handler(fw::gui::widget *wdgt) {
  std::shared_ptr<fw::particle_effect> effect =
      fw::framework::get_instance()->get_particle_mgr()->create_effect("explosion-01");
  return true;
}

bool pause_handler(fw::gui::widget *wdgt) {
  fw::gui::button *btn = dynamic_cast<fw::gui::button *>(wdgt);
  fw::framework *framework = fw::framework::get_instance();
  if (framework->is_paused()) {
    framework->unpause();
    btn->set_text("Pause");
  } else {
    framework->pause();
    btn->set_text("Unpause");
  }

  return true;
}

bool application::initialize(fw::framework *frmwrk) {
  fw::top_down_camera *cam = new fw::top_down_camera();
  cam->set_mouse_move(false);
  frmwrk->set_camera(cam);

  fw::gui::window *wnd;
  wnd = fw::gui::builder<fw::gui::window>()
      << fw::gui::widget::position(fw::gui::px(20), fw::gui::px(20))
      << fw::gui::widget::size(fw::gui::px(150), fw::gui::px(100))
      << fw::gui::window::background("frame")
      << (fw::gui::builder<fw::gui::button>()
          << fw::gui::widget::position(fw::gui::px(30), fw::gui::px(30))
          << fw::gui::widget::size(fw::gui::px(130), fw::gui::px(30))
          << fw::gui::button::text("Restart")
          << fw::gui::widget::click(std::bind<bool>(click_handler, std::placeholders::_1)))
      << (fw::gui::builder<fw::gui::button>()
          << fw::gui::widget::position(fw::gui::px(30), fw::gui::px(70))
          << fw::gui::widget::size(fw::gui::px(130), fw::gui::px(30))
          << fw::gui::button::text("Pause")
          << fw::gui::widget::click(std::bind<bool>(pause_handler, std::placeholders::_1)));
  frmwrk->get_gui()->attach_widget(wnd);

  std::shared_ptr<fw::particle_effect> effect =
      frmwrk->get_particle_mgr()->create_effect("explosion-01");
  return true;
}

void application::update(float dt) {
}

void application::render(fw::sg::scenegraph &scenegraph) {
  scenegraph.set_clear_colour(fw::colour(1, 0, 0.0, 0));

  FW_CHECKED(glDisable(GL_DEPTH_TEST));
  FW_CHECKED(glBlendFunc(GL_ONE, GL_ONE));

}

//-----------------------------------------------------------------------------

int main(int argc, char** argv) {
  try {
    settings_initialize(argc, argv);

    application app;
    new fw::framework(&app);
    fw::framework::get_instance()->initialize("Particle Test");
    fw::framework::get_instance()->run();
  } catch (std::exception &e) {
    std::string msg = boost::diagnostic_information(e);
    fw::debug
        << "--------------------------------------------------------------------------------"
        << std::endl;
    fw::debug << "UNHANDLED EXCEPTION!" << std::endl;
    fw::debug << msg << std::endl;

    display_exception(e.what());
  } catch (...) {
    fw::debug
        << "--------------------------------------------------------------------------------"
        << std::endl;
    fw::debug << "UNHANDLED EXCEPTION! (unknown exception)" << std::endl;
  }

  return 0;
}

void display_exception(std::string const &msg) {
  std::stringstream ss;
  ss
      << "An error has occurred. Please send your log file (below) to dean@codeka.com.au for diagnostics."
      << std::endl;
  ss << std::endl;
  ss << fw::debug.get_filename() << std::endl;
  ss << std::endl;
  ss << msg;
}

void settings_initialize(int argc, char** argv) {
  po::options_description options("Additional options");
  options.add_options()("particle-file",
      po::value<std::string>()->default_value("explosion-01"),
      "Name of the particle file to load, we assume it can be fw::resolve'd.");

  fw::settings::initialize(options, argc, argv, "font-test.conf");
}

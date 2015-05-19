#include <iostream>

#include <boost/program_options.hpp>

#include <framework/bitmap.h>
#include <framework/camera.h>
#include <framework/settings.h>
#include <framework/framework.h>
#include <framework/font.h>
#include <framework/logging.h>
#include <framework/particle_manager.h>
#include <framework/particle_effect.h>
#include <framework/misc.h>
#include <framework/paths.h>
#include <framework/scenegraph.h>

namespace po = boost::program_options;

void settings_initialize(int argc, char** argv);
void display_exception(std::string const &msg);

class application : public fw::base_app {
public:
  bool initialize(fw::framework *frmwrk);
  void update(float dt);
  void render(fw::sg::scenegraph &scenegraph);
};

bool application::initialize(fw::framework *frmwrk) {
  fw::top_down_camera *cam = new fw::top_down_camera();
  cam->set_mouse_move(false);
  frmwrk->set_camera(cam);

  std::shared_ptr<fw::particle_effect> effect =
      frmwrk->get_particle_mgr()->create_effect("explosion-01");
  //effect->
  return true;
}

void application::update(float dt) {
}

void application::render(fw::sg::scenegraph &scenegraph) {
}


//-----------------------------------------------------------------------------

int main(int argc, char** argv) {
  try {
    settings_initialize(argc, argv);

    application app;
    new fw::framework(&app);
    fw::framework::get_instance()->initialize("Particle Test");
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
}

void settings_initialize(int argc, char** argv) {
  po::options_description options("Additional options");
  options.add_options()
      ("particle-file", po::value<std::string>()->default_value("explosion-01"), "Name of the particle file to load, we assume it can be fw::resolve'd.")
    ;

  fw::settings::initialize(options, argc, argv, "font-test.conf");
}

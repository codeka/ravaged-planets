
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/timer.h>
#include <framework/input.h>
#include <framework/camera.h>
#include <framework/logging.h>

#include <game/application.h>
//#include <warworlds/screens/screen.h>
//#include <warworlds/session/session.h>
//#include <warworlds/simulation/simulation_thread.h>

namespace rp {

application::application()
  : _framework(nullptr), _screen(nullptr) {
}

application::~application() {
}

bool application::initialize(fw::framework *frmwrk) {
  _framework = frmwrk;
//  _framework->get_input()->show_cursor();

  // set up the camera
//  fw::top_down_camera *cam = new fw::top_down_camera();
//  cam->set_mouse_move(false);
//  _framework->set_camera(cam);

  // start the simulation thread now, it'll always run even if there's
  // no actual game running....
//  simulation_thread::get_instance()->initialise();
  /*
   // attach our GUI sounds to the various events
   fw::audio_manager *audio_mgr = _framework->get_audio();
   fw::gui::system_sounds *sounds = _framework->get_gui()->get_system_sounds();
   sounds->attach_sound(fw::gui::system_sounds::button_hover,
   audio_mgr->load_file("ui/button-hover.ogg"));
   sounds->attach_sound(fw::gui::system_sounds::button_click,
   audio_mgr->load_file("ui/button-click.ogg"));
   */
//  _screen = new screen_stack();
//  _screen->set_active_screen("title");

  return true;
}

void application::destroy() {
  fw::debug << "application shutting down." << std::endl;
//  simulation_thread::get_instance()->destroy();
}

void application::update(float dt) {
//  _screen->get_active_screen()->update(dt);
//  session::get_instance()->update(dt);
}

void application::render(fw::sg::scenegraph &scenegraph) {
//  _screen->get_active_screen()->render(scenegraph);
}

}

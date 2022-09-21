
#include <memory>

#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/timer.h>
#include <framework/input.h>
#include <framework/cursor.h>
#include <framework/camera.h>
#include <framework/logging.h>

#include <game/application.h>
#include <game/screens/screen.h>
#include <game/session/session.h>
#include <game/simulation/simulation_thread.h>

namespace game {

application::application()
  : _framework(nullptr), _screen(nullptr) {
}

application::~application() {
}

bool application::initialize(fw::framework *frmwrk) {
  _framework = frmwrk;
  _framework->get_cursor()->set_visible(true);

  // set up the camera
  fw::TopDownCamera *cam = new fw::TopDownCamera();
  cam->set_mouse_move(false);
  _framework->set_camera(cam);

  // start the simulation thread now, it'll always run even if there's
  // no actual game running....
  simulation_thread::get_instance()->initialize();
  /*
   // attach our GUI sounds to the various events
   fw::audio_manager *audio_mgr = _framework->get_audio();
   fw::gui::system_sounds *sounds = _framework->get_gui()->get_system_sounds();
   sounds->attach_sound(fw::gui::system_sounds::button_hover,
   audio_mgr->load_file("ui/button-hover.ogg"));
   sounds->attach_sound(fw::gui::system_sounds::button_click,
   audio_mgr->load_file("ui/button-click.ogg"));
   */
  _screen = new screen_stack();
  _screen->set_active_screen("title");

  return true;
}

void application::destroy() {
  fw::debug << "application shutting down." << std::endl;
  simulation_thread::get_instance()->destroy();
}

void application::update(float dt) {
  screen *active = _screen->get_active_screen();
  if (active != nullptr) {
    active->update();
  }
  session::get_instance()->update();
}

void application::render(fw::sg::Scenegraph &Scenegraph) {
  screen *active = _screen->get_active_screen();
  if (active != nullptr) {
    active->render(Scenegraph);
  }
}

}

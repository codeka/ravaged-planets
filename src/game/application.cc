
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

Application::Application()
  : framework_(nullptr), screen_stack_(nullptr) {
}

Application::~Application() {
}

fw::Status Application::initialize(fw::Framework * framework) {
  framework_ = framework;
  framework_->get_cursor()->set_visible(true);

  // set up the camera
  fw::TopDownCamera *cam = new fw::TopDownCamera();
  cam->set_mouse_move(false);
  framework_->set_camera(cam);

  // start the simulation thread now, it'll always run even if there's
  // no actual game running....
  SimulationThread::get_instance()->initialize();
  /*
   // attach our GUI sounds to the various events
   fw::audio_manager *audio_mgr = framework_->get_audio();
   fw::gui::system_sounds *sounds = framework_->get_gui()->get_system_sounds();
   sounds->attach_sound(fw::gui::system_sounds::button_hover,
   audio_mgr->load_file("ui/button-hover.ogg"));
   sounds->attach_sound(fw::gui::system_sounds::button_click,
   audio_mgr->load_file("ui/button-click.ogg"));
   */
  screen_stack_ = new ScreenStack();
  screen_stack_->set_active_screen("title");

  return fw::OkStatus();
}

void Application::destroy() {
  fw::debug << "application shutting down." << std::endl;
  SimulationThread::get_instance()->destroy();
}

void Application::update(float dt) {
  Screen *active = screen_stack_->get_active_screen();
  if (active != nullptr) {
    active->update();
  }
  Session::get_instance()->update();
}

}

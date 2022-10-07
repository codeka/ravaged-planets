
#include <framework/camera.h>
#include <framework/exception.h>
#include <framework/framework.h>
#include <framework/scenegraph.h>

#include <game/world/terrain.h>
#include <game/world/world.h>
#include <game/world/world_reader.h>
#include <game/simulation/simulation_thread.h>
#include <game/simulation/player.h>
#include <game/screens/hud/build_window.h>
#include <game/screens/hud/minimap_window.h>
#include <game/screens/hud/pause_window.h>
#include <game/screens/game_screen.h>

namespace game {

GameScreen::GameScreen() : world_(nullptr) {
  hud_build = new BuildWindow();
  hud_minimap = new MinimapWindow();
  hud_pause = new PauseWindow();

  hud_build->initialize();
  hud_minimap->initialize();
  hud_pause->initialize();
}

GameScreen::~GameScreen() {
  delete world_;
  delete hud_pause;
  delete hud_minimap;
  delete hud_build;
}

void GameScreen::set_options(std::shared_ptr<ScreenOptions> opt) {
  options_ = std::dynamic_pointer_cast<GameScreenOptions>(opt);
}

void GameScreen::show() {
  if (options_ == 0) {
    BOOST_THROW_EXCEPTION(
        fw::Exception() << fw::message_error_info("no GameScreenOptions has been set, cannot start new game!"));
  }

  std::shared_ptr<WorldReader> reader(new WorldReader());
  reader->read(options_->map_name);
  world_ = new World(reader);

  // initialize the world
  world_->initialize();

  // notify all of the players that the world is loaded
  for(Player * plyr : SimulationThread::get_instance()->get_players()) {
    plyr->world_loaded();
  }

  // show the initial set of windows
//  hud_chat->show();
  hud_minimap->show();
}

void GameScreen::update() {
  world_->update();

  hud_build->update();
  hud_minimap->update();
}

void GameScreen::render(fw::sg::Scenegraph &scenegraph) {
  if (world_ == nullptr) {
    return;
  }

  // set up the properties of the sun that we'll use to Light and also cast shadows
  fw::Vector sun(0.485f, 0.485f, 0.727f);
  fw::Camera *old_cam = fw::Framework::get_instance()->get_camera();
  fw::Vector cam_pos = old_cam->get_position();
  fw::Vector cam_dir = old_cam->get_forward();
  fw::Vector lookat = world_->get_terrain()->get_cursor_location(cam_pos, cam_dir);

  std::shared_ptr <fw::sg::Light> light(new fw::sg::Light(sun * 200.0f, sun * -1, true));
  scenegraph.add_light(light);

  world_->render(scenegraph);
}

void GameScreen::hide() {
  world_->destroy();

  hud_minimap->hide();
}

}


#include <framework/camera.h>
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
  if (options_ == nullptr) {
    fw::debug << "ERROR no GameScreenOptions has been set, cannot start new game!" << std::endl;
    return;
  }

  std::shared_ptr<WorldReader> reader(new WorldReader());
  auto status = reader->Read(options_->map_name);
  if (!status.ok()) {
    // TODO: make this return the error the caller instead
    fw::debug << "Error loading world: " << status << std::endl;
  }
  world_ = new World(reader);

  // initialize the world
  world_->initialize();

  // notify all of the players that the world is loaded
  for(auto plyr : SimulationThread::get_instance()->get_players()) {
    plyr->world_loaded();
  }

  // show the initial set of windows
//  hud_chat->show();
  hud_minimap->show();

  // Add a light to the scene. TODO: remove it on close? also update it somehow?
  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue([=](fw::sg::Scenegraph& sg) {
      fw::Vector sun(0.485f, 0.485f, 100.727f);
      std::shared_ptr <fw::sg::Light> light(new fw::sg::Light(sun * 200.0f, sun * -1, true));
      sg.add_light(light);
    });
}

void GameScreen::update() {
  world_->update();

  hud_build->update();
  hud_minimap->update();
}

void GameScreen::hide() {
  world_->destroy();

  hud_minimap->hide();
}

}


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

game_screen::game_screen() : _world(nullptr) {
  hud_build = new build_window();
  hud_minimap = new minimap_window();
  hud_pause = new pause_window();

  hud_build->initialize();
  hud_minimap->initialize();
  hud_pause->initialize();
}

game_screen::~game_screen() {
  delete _world;
  delete hud_pause;
  delete hud_minimap;
  delete hud_build;
}

void game_screen::set_options(std::shared_ptr<screen_options> opt) {
  _options = std::dynamic_pointer_cast<game_screen_options>(opt);
}

void game_screen::show() {
  if (_options == 0) {
    BOOST_THROW_EXCEPTION(
        fw::Exception() << fw::message_error_info("no game_screen_options has been set, cannot start new game!"));
  }

  std::shared_ptr<world_reader> reader(new world_reader());
  reader->read(_options->map_name);
  _world = new world(reader);

  // initialize the world
  _world->initialize();

  // notify all of the players that the world is loaded
  for(player * plyr : simulation_thread::get_instance()->get_players()) {
    plyr->world_loaded();
  }

  // show the initial set of windows
//  hud_chat->show();
  hud_minimap->show();
}

void game_screen::update() {
  _world->update();

  hud_build->update();
  hud_minimap->update();
}

void game_screen::render(fw::sg::Scenegraph &Scenegraph) {
  if (_world == nullptr) {
    return;
  }

  // set up the properties of the sun that we'll use to Light and also cast shadows
  fw::Vector sun(0.485f, 0.485f, 0.727f);
  fw::Camera *old_cam = fw::Framework::get_instance()->get_camera();
  fw::Vector cam_pos = old_cam->get_position();
  fw::Vector cam_dir = old_cam->get_forward();
  fw::Vector lookat = _world->get_terrain()->get_cursor_location(cam_pos, cam_dir);

  std::shared_ptr <fw::sg::Light> Light(new fw::sg::Light(sun * 200.0f, sun * -1, true));
  Scenegraph.add_light(Light);

  _world->render(Scenegraph);
}

void game_screen::hide() {
  _world->destroy();

  hud_minimap->hide();
}

}

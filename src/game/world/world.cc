
#include <functional>

#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>

#include <framework/bitmap.h>
#include <framework/framework.h>
#include <framework/logging.h>
#include <framework/input.h>
#include <framework/particle_manager.h>
#include <framework/camera.h>
#include <framework/paths.h>
#include <framework/scenegraph.h>
#include <game/world/world.h>
#include <game/world/world_reader.h>
#include <game/world/terrain.h>
//#include "../entities/entity.h"
//#include "../entities/entity_factory.h"
//#include "../entities/entity_manager.h"
//#include "../entities/moveable_component.h"
//#include "../entities/position_component.h"
//#include "../entities/weapon_component.h"
//#include "../simulation/simulation_thread.h"
//#include "../screens/hud/pause_window.h"
//#include "../session/session.h"
//#include "../ai/pathing_thread.h"

namespace fs = boost::filesystem;
using namespace std::placeholders;

namespace rp {

//-------------------------------------------------------------------------
world *world::_instance = nullptr;

world::world(std::shared_ptr<world_reader> reader) :
    _reader(reader), /*_entities(0),*/ _terrain(nullptr)/*, _pathing(0)*/, _initialized(false) {
  //_cursor = new cursor_handler();
}

world::~world() {
  world::set_instance(nullptr);
 // delete _cursor;
 // if (_pathing != nullptr)
 //   delete _pathing;
}

void world::initialize() {
  _terrain = _reader->get_terrain();
 // _minimap_background = _reader->get_minimap_background();
  _name = _reader->get_name();
  _description = _reader->get_description();
  _author = _reader->get_author();

  initialize_pathing();

  _terrain->initialize();

  BOOST_FOREACH(auto it, _reader->get_player_starts()) {
    _player_starts[it.first] = it.second;
  }

  initialize_entities();

  // tell the particle manager to wrap particles at the world boundary
  fw::framework::get_instance()->get_particle_mgr()->set_world_wrap(
      _terrain->get_width(), _terrain->get_length());

  fw::input *input = fw::framework::get_instance()->get_input();
/*
  if (_entities != nullptr) {
    _cursor->initialise();

    _keybind_tokens.push_back(
        input->bind_function("pause",
            boost::bind(&world::on_key_pause, this, _1, _2)));
  }*/
  _keybind_tokens.push_back(input->bind_function("screenshot", std::bind(&world::on_key_screenshot, this, _1, _2)));
  _initialized = true;

  world::set_instance(this);
}

void world::destroy() {
//  _pathing->stop();

  // unbind all the keys we had bound
  fw::input *input = fw::framework::get_instance()->get_input();
  BOOST_FOREACH(int token, _keybind_tokens) {
    input->unbind_key(token);
  }
  _keybind_tokens.clear();
//  _cursor->destroy();
}

void world::initialize_pathing() {
//  _pathing = new pathing_thread();
//  _pathing->start();
}

void world::initialize_entities() {
//  _entities = new ent::entity_manager();
//  _entities->initialise();
}

// this is called when the "pause" button is pressed (usually "ESC")
void world::on_key_pause(std::string, bool is_down) {
  if (!is_down) {
    if (fw::framework::get_instance()->is_paused()) {
      fw::framework::get_instance()->unpause();
    } else {
//      hud_pause->show();
      fw::framework::get_instance()->pause();
    }
  }
}

void world::on_key_screenshot(std::string, bool is_down) {
  if (!is_down) {
    fw::framework::get_instance()->take_screenshot(0, 0,
        std::bind(&world::screenshot_callback, this, std::placeholders::_1));
  }
}

void world::screenshot_callback(fw::bitmap const &screenshot) {
  // screenshots go under the data directory\screens folder
  fs::path base_path = fw::resolve("screens", true);
  if (!fs::exists(base_path)) {
    fs::create_directories(base_path);
  }

  // create a file called "screen-nnnn.png" where nnnn is a number from 1 to 9999 and make
  // sure the name is unique
  int max_file_number = 0;
  for (fs::directory_iterator it(base_path); it != fs::directory_iterator(); ++it) {
    fs::path p(*it);
    if (fs::is_regular_file(p)) {
      std::string filename = p.leaf().string();
      if (boost::istarts_with(filename, "screen-") && boost::iends_with(filename, ".png")) {
        std::string number_part = filename.substr(7, filename.length() - 11);
        try {
          int number = boost::lexical_cast<int>(number_part);
          if (number > max_file_number) {
            max_file_number = number;
          }
        } catch (boost::bad_lexical_cast &) {
          // Ignore, keep looking?
        }
      }
    }
  }

  fs::path full_path = base_path / (boost::format("screen-%1$04d.png") % (max_file_number + 1)).str();
  screenshot.save_bitmap(full_path.string());
}

void world::update() {
  if (!_initialized) {
    return;
  }
  // if update is called, we can't be paused so hide the "pause" menu...
//  if (hud_pause != 0 && hud_pause->is_visible())
//    hud_pause->hide();

// _cursor->update();
  _terrain->update();

 // if (_entities != 0)
 //   _entities->update();
}

void world::render(fw::sg::scenegraph &scenegraph) {
  if (!_initialized) {
    return;
  }

  _terrain->render(scenegraph);

 // if (_entities != nullptr) {
 //   _entities->render(scenegraph);
  // }
}

}

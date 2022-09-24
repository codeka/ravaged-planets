
#include <functional>

#include <boost/filesystem.hpp>

#include <framework/bitmap.h>
#include <framework/framework.h>
#include <framework/logging.h>
#include <framework/input.h>
#include <framework/particle_manager.h>
#include <framework/camera.h>
#include <framework/paths.h>
#include <framework/scenegraph.h>

#include <game/ai/pathing_thread.h>
#include <game/entities/entity_manager.h>
#include <game/entities/ownable_component.h>
#include <game/world/cursor_handler.h>
#include <game/world/world.h>
#include <game/world/world_reader.h>
#include <game/world/terrain.h>
#include <game/screens/hud/pause_window.h>
#include <game/simulation/simulation_thread.h>
#include <game/simulation/local_player.h>

namespace fs = boost::filesystem;
using namespace std::placeholders;

namespace game {

//-------------------------------------------------------------------------
world *world::_instance = nullptr;

world::world(std::shared_ptr<world_reader> reader) :
    _reader(reader), _entities(nullptr), _terrain(nullptr), _pathing(nullptr), _initialized(false) {
  cursor_ = new cursor_handler();
}

world::~world() {
  world::set_instance(nullptr);
  delete cursor_;
  if (_pathing != nullptr)
    delete _pathing;
}

void world::initialize() {
  _terrain = _reader->get_terrain();
  _minimap_background = _reader->get_minimap_background();
  _name = _reader->get_name();
  _description = _reader->get_description();
  _author = _reader->get_author();
  _screenshot = _reader->get_screenshot();

  _terrain->initialize();

  for (auto it : _reader->get_player_starts()) {
    _player_starts[it.first] = it.second;
  }

  // tell the Particle manager to wrap particles at the world boundary
  fw::Framework::get_instance()->get_particle_mgr()->set_world_wrap(_terrain->get_width(), _terrain->get_length());

  fw::Input *input = fw::Framework::get_instance()->get_input();

  world::set_instance(this);

  initialize_entities();
  if (_entities != nullptr) {
    cursor_->initialize();
    _keybind_tokens.push_back(input->bind_function("pause", std::bind(&world::on_key_pause, this, _1, _2)));
  }
  _keybind_tokens.push_back(input->bind_function("screenshot", std::bind(&world::on_key_screenshot, this, _1, _2)));

  initialize_pathing();

  _initialized = true;
}

void world::destroy() {
  _pathing->stop();

  // unbind all the keys we had bound
  fw::Input *Input = fw::Framework::get_instance()->get_input();
  for (int token : _keybind_tokens) {
    Input->unbind_key(token);
  }
  _keybind_tokens.clear();
  cursor_->destroy();
}

void world::initialize_pathing() {
  _pathing = new pathing_thread();
  _pathing->start();
}

void world::initialize_entities() {
  _entities = new ent::entity_manager();
  _entities->initialize();
}

// this is called when the "pause" button is pressed (usually "ESC")
void world::on_key_pause(std::string, bool is_down) {
  if (!is_down) {
    if (fw::Framework::get_instance()->is_paused()) {
      fw::Framework::get_instance()->unpause();
    } else {
      hud_pause->show();
      fw::Framework::get_instance()->pause();
    }
  }
}

void world::on_key_screenshot(std::string, bool is_down) {
  if (!is_down) {
    fw::Framework::get_instance()->take_screenshot(0, 0, std::bind(&world::screenshot_callback, this, _1));
  }
}

void world::screenshot_callback(std::shared_ptr<fw::Bitmap> screenshot) {
  // screenshots go under the data directory\screens folder
  fs::path base_path = fw::resolve("screens", true);
  if (!fs::exists(base_path)) {
    fs::create_directories(base_path);
  }

  // create a file called "Screen-nnnn.png" where nnnn is a number from 1 to 9999 and make
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
  screenshot->save_bitmap(full_path.string());
}

void world::update() {
  if (!_initialized) {
    return;
  }
  // if update is called, we can't be paused so hide the "pause" menu...
  if (hud_pause != nullptr && hud_pause->is_visible()) {
    hud_pause->hide();
  }

  cursor_->update();
  _terrain->update();

  if (_entities != nullptr)
    _entities->update();
}

void world::render(fw::sg::Scenegraph &Scenegraph) {
  if (!_initialized) {
    return;
  }

  _terrain->render(Scenegraph);

  if (_entities != nullptr) {
    _entities->render(Scenegraph);
  }
}

}

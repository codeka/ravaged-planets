#pragma once

#include <map>
#include <memory>

#include <framework/vector.h>

namespace fw {
class Graphics;
class Bitmap;
class texture;

namespace sg {
class Scenegraph;
}
}

namespace ent {
class EntityManager;
}

namespace game {
class terrain;
class world_reader;
class cursor_handler;
class pathing_thread;

/**
 * The world class represents the entire game "world", that is, the terrain the trees/obstacles and all the units.
 */
class world {
private:
  std::shared_ptr<world_reader> _reader;
  terrain *_terrain;
  ent::EntityManager *_entities;
  pathing_thread *_pathing;
  std::vector<int> _keybind_tokens;
  cursor_handler *cursor_;
  std::shared_ptr<fw::Bitmap> _minimap_background;
  std::map<int, fw::Vector> _player_starts;

  std::string _description;
  std::string name_;
  std::string _author;
  bool _initialized;

  static world *_instance;

  void on_key_pause(std::string key, bool is_down);
  void on_key_screenshot(std::string key, bool is_down);

  // This is called after a screenshot is taken, we'll save it to disk.
  void screenshot_callback(std::shared_ptr<fw::Bitmap> screenshot);

protected:
  std::shared_ptr<fw::Bitmap> _screenshot;

  virtual void initialize_pathing();
  virtual void initialize_entities();

public:
  // constructs a new world using the information from the given world_reader.
  world(std::shared_ptr<world_reader> reader);
  virtual ~world();

  virtual void initialize();
  virtual void destroy();
  virtual void update();
  virtual void render(fw::sg::Scenegraph &Scenegraph);

  void pause();
  void unpause();

  static world *get_instance() {
    return _instance;
  }
  static void set_instance(world *wrld) {
    _instance = wrld;
  }

  std::shared_ptr<fw::Bitmap> const &get_minimap_background() const {
    return _minimap_background;
  }

  std::shared_ptr<fw::Bitmap> get_screenshot() const {
    return _screenshot;
  }

  std::map<int, fw::Vector> &get_player_starts() {
    return _player_starts;
  }

  terrain *get_terrain() const {
    return _terrain;
  }
  ent::EntityManager *get_entity_manager() const {
    return _entities;
  }
  pathing_thread *get_pathing() const {
    return _pathing;
  }

  std::string get_description() const {
    return _description;
  }
  void set_description(std::string description) {
    _description = description;
  }

  std::string get_name() const {
    return name_;
  }
  void set_name(std::string name) {
    name_ = name;
  }

  std::string get_author() const {
    return _author;
  }
  void set_author(std::string author) {
    _author = author;
  }
};

}

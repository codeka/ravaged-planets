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
class Terrain;
class WorldReader;
class CursorHandler;
class PathingThread;

/**
 * The world class represents the entire game "world", that is, the terrain the trees/obstacles and all the units.
 */
class World {
private:
  std::shared_ptr<WorldReader> reader_;
  Terrain *terrain_;
  ent::EntityManager *entities_;
  PathingThread *pathing_;
  std::vector<int> keybind_tokens_;
  CursorHandler *cursor_;
  std::shared_ptr<fw::Bitmap> minimap_background_;
  std::map<int, fw::Vector> player_starts_;

  std::string description_;
  std::string name_;
  std::string author_;
  bool initialized_;

  static World *instance_;

  void on_key_pause(std::string key, bool is_down);
  void on_key_screenshot(std::string key, bool is_down);

  // This is called after a screenshot is taken, we'll save it to disk.
  void screenshot_callback(std::shared_ptr<fw::Bitmap> screenshot);

protected:
  std::shared_ptr<fw::Bitmap> screenshot_;

  virtual void initialize_pathing();
  virtual void initialize_entities();

public:
  // constructs a new world using the information from the given world_reader.
  World(std::shared_ptr<WorldReader> reader);
  virtual ~World();

  virtual void initialize();
  virtual void destroy();
  virtual void update();
  virtual void render(fw::sg::Scenegraph &Scenegraph);

  void pause();
  void unpause();

  static World *get_instance() {
    return instance_;
  }
  static void set_instance(World *wrld) {
    instance_ = wrld;
  }

  std::shared_ptr<fw::Bitmap> const &get_minimap_background() const {
    return minimap_background_;
  }

  std::shared_ptr<fw::Bitmap> get_screenshot() const {
    return screenshot_;
  }

  std::map<int, fw::Vector> &get_player_starts() {
    return player_starts_;
  }

  Terrain *get_terrain() const {
    return terrain_;
  }
  ent::EntityManager *get_entity_manager() const {
    return entities_;
  }
  PathingThread *get_pathing() const {
    return pathing_;
  }

  std::string get_description() const {
    return description_;
  }
  void set_description(std::string description) {
    description_ = description;
  }

  std::string get_name() const {
    return name_;
  }
  void set_name(std::string name) {
    name_ = name;
  }

  std::string get_author() const {
    return author_;
  }
  void set_author(std::string author) {
    author_ = author;
  }
};

}

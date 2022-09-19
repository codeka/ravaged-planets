#pragma once

#include <memory>

#include <framework/vector.h>

namespace fw {
class Bitmap;
class xml_element;
}

namespace game {

class terrain;
class world;
class world_file_entry;

// this class reads the map from the filesystem and lets the world populate itself.
class world_reader {
protected:
  std::shared_ptr<fw::Bitmap> _minimap_background;
  std::shared_ptr<fw::Bitmap> _screenshot;
  terrain *_terrain;
  virtual terrain *create_terrain(int width, int length);

  std::map<int, fw::vector> _player_starts;
  std::string _name;
  std::string _description;
  std::string _author;

  void read_mapdesc(fw::xml_element root);
  void read_mapdesc_players(fw::xml_element players_node);
  void read_collision_data(world_file_entry &wfe);

public:
  world_reader();
  virtual ~world_reader();

  // reads the map with the given name and populates our members
  void read(std::string name);

  // gets the various things that we loaded from the map file(s), so that the world
  // can populate itself
  terrain *get_terrain();

  std::map<int, fw::vector> const &get_player_starts() const {
    return _player_starts;
  }
  std::shared_ptr<fw::Bitmap> get_minimap_background() const {
    return _minimap_background;
  }
  std::shared_ptr<fw::Bitmap> get_screenshot() const {
    return _screenshot;
  }
  std::string get_name() const {
    return _name;
  }
  std::string get_description() const {
    return _description;
  }
  std::string get_author() const {
    return _author;
  }
};

}

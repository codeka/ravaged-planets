#pragma once

#include <memory>

#include <framework/vector.h>

namespace fw {
class Bitmap;
class XmlElement;
}

namespace game {

class Terrain;
class World;
class WorldFileEntry;

// this class reads the map from the filesystem and lets the world populate itself.
class WorldReader {
protected:
  std::shared_ptr<fw::Bitmap> minimap_background_;
  std::shared_ptr<fw::Bitmap> screenshot_;
  Terrain *terrain_;
  std::map<int, fw::Vector> player_starts_;
  std::string name_;
  std::string description_;
  std::string author_;

  virtual Terrain* create_terrain(int width, int length);

  void read_mapdesc(fw::XmlElement root);
  void read_mapdesc_players(fw::XmlElement players_node);
  void read_collision_data(WorldFileEntry &wfe);

public:
  WorldReader();
  virtual ~WorldReader();

  // reads the map with the given name and populates our members
  void read(std::string name);

  // gets the various things that we loaded from the map file(s), so that the world
  // can populate itself
  Terrain *get_terrain();

  std::map<int, fw::Vector> const &get_player_starts() const {
    return player_starts_;
  }
  std::shared_ptr<fw::Bitmap> get_minimap_background() const {
    return minimap_background_;
  }
  std::shared_ptr<fw::Bitmap> get_screenshot() const {
    return screenshot_;
  }
  std::string get_name() const {
    return name_;
  }
  std::string get_description() const {
    return description_;
  }
  std::string get_author() const {
    return author_;
  }
};

}

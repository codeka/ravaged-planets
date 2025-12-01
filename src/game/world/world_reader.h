#pragma once

#include <memory>

#include <framework/bitmap.h>
#include <framework/math.h>
#include <framework/status.h>
#include <framework/xml.h>

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
  fw::Bitmap minimap_background_;
  fw::Bitmap screenshot_;
  std::shared_ptr<Terrain> terrain_;
  std::map<int, fw::Vector> player_starts_;
  std::string name_;
  std::string description_;
  std::string author_;

  virtual fw::StatusOr<std::shared_ptr<Terrain>> create_terrain(
      int width, int length, float* height_data);

  fw::Status ReadMapdesc(fw::XmlElement root);
  fw::Status ReadMapdescPlayers(fw::XmlElement players_node);
  fw::Status ReadCollisionData(WorldFileEntry &wfe);

public:
  WorldReader();
  virtual ~WorldReader();

  // reads the map with the given name and populates our members
  fw::Status Read(std::string name);

  std::shared_ptr<Terrain> get_terrain() const {
    return terrain_;
  }
  std::map<int, fw::Vector> const &get_player_starts() const {
    return player_starts_;
  }
  fw::Bitmap const &get_minimap_background() const {
    return minimap_background_;
  }
  fw::Bitmap const &get_screenshot() const {
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

#pragma once

#include <framework/color.h>

namespace game {
class WorldFile;
}

namespace ed {
class EditorWorld;

// This class writes all the information for a given world (map) into the given directory or .map file (which is
// just a zipped version of the directory)
class WorldWriter {
private:
  EditorWorld *world_;
  std::string name_;
  fw::Color base_minimap_colors_[4];

  fw::Color get_terrain_color(int x, int z);
  void calculate_base_minimap_colors();

  void write_terrain(game::WorldFile &wf);
  void write_mapdesc(game::WorldFile &wf);
  void write_minimap_background(game::WorldFile &wf);
  void write_collision_data(game::WorldFile &wf);

public:
  WorldWriter(EditorWorld *wrld);
  ~WorldWriter();

  void write(std::string name);
};

}

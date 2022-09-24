#pragma once

#include <framework/color.h>

namespace game {
class world_file;
}

namespace ed {
class editor_world;

/** This class writes all the information for a given world (map) into the given directory or .map file (which is
 * just a zipped version of the directory)
 */
class world_writer {
private:
  editor_world *world_;
  std::string _name;
  fw::Color _base_minimap_colors[4];

  fw::Color get_terrain_color(int x, int z);
  void calculate_base_minimap_colors();

  void write_terrain(game::world_file &wf);
  void write_mapdesc(game::world_file &wf);
  void write_minimap_background(game::world_file &wf);
  void write_collision_data(game::world_file &wf);

public:
  world_writer(editor_world *wrld);
  ~world_writer();

  void write(std::string name);
};

}

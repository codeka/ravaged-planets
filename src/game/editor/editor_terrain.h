#pragma once

#include <atomic>
#include <mutex>
#include <tuple>

#include <game/world/terrain.h>

namespace fw {
class Bitmap;
class texture;
}

namespace ed {

// this subclass of game::Terrain allows us to edit the actual heightfield data, textures, and so on.
class EditorTerrain: public game::Terrain {
private:
  std::mutex patches_to_bake_mutex_;
  std::vector<std::tuple<int, int>> patches_to_bake_;
  std::atomic<bool> bake_queued_;

  // we keep a separate vector of the splatt bitmaps for easy editing
  std::vector<fw::Bitmap> splatt_bitmaps_;

  // We also keep a separate vector of the layer bitmaps
  std::vector<std::shared_ptr<fw::Bitmap>> layer_bitmaps_;

public:
  EditorTerrain(int width, int height, float* height_data = nullptr);
  virtual ~EditorTerrain();

  // set a new height for the given vertex
  void set_vertex_height(int x, int y, float height);

  // gets and sets the texture of the given layer.
  int get_num_layers() const;
  std::shared_ptr<fw::Bitmap> get_layer(int number);
  void set_layer(int number, std::shared_ptr<fw::Bitmap> bitmap);

  // creates all of the splatt textures and sets them up initially
  void initialize_splatt();

  // sets the splat texture for the given patch to the given bitmap
  virtual void set_splatt(int patch_x, int patch_z, fw::Bitmap const &bmp);
  fw::Bitmap &get_splatt(int patch_x, int patch_z);

  float *get_height_data() const {
    return heights_;
  }

  // builds the collision data for the whole map. we assume the vertices buffer
  // is big enough to hold one data point per vertex in the map. each data point
  // holds a single boolean flag - true means "passable", false means "impassable"
  void build_collision_data(std::vector<bool> &vertices);
};

}

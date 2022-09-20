#pragma once

#include <memory>
#include <vector>
#include <stdint.h>

#include <framework/bitmap.h>
#include <framework/texture.h>
#include <framework/vector.h>

namespace fw {
class VertexBuffer;
class IndexBuffer;
class shader;
class shader_parameters;

namespace sg {
class scenegraph;
}
}

namespace ed {
class world_writer;
}

namespace game {

struct terrain_patch {
  std::shared_ptr<fw::VertexBuffer> vb;
  std::shared_ptr<fw::Texture> texture;
  std::shared_ptr<fw::shader_parameters> shader_params;
};

class terrain {
public:
  static const int PATCH_SIZE = 64;

private:
  std::vector<std::shared_ptr<terrain_patch> > _patches;
  std::shared_ptr<fw::IndexBuffer> _ib;
  std::shared_ptr<fw::shader> _shader;

protected:
  friend class ed::world_writer;
  friend class world_reader;

  std::vector<std::shared_ptr<fw::Texture>> _layers;
  std::vector<bool> _collision_data;

  int _width;
  int _length;
  float *_heights;

  // get the width/height of the terrain in patches
  int get_patches_width() {
    return (_width / PATCH_SIZE);
  }
  int get_patches_length() {
    return (_length / PATCH_SIZE);
  }

  // Gets the index of the given x/z coordinates for a single patch
  int get_patch_index(int patch_x, int patch_z, int *new_patch_x = 0,
      int *new_patch_z = 0);

  // bake a patch's height values into a texture, ready to be
  // passed to our vertex shader. cool!
  void bake_patch(int patch_x, int patch_z);

  // gets or sets the splatt texture for the given patch
  std::shared_ptr<fw::Texture> get_patch_splatt(int patch_x, int patch_z);
  virtual void set_patch_splatt(int patch_x, int patch_z,
      std::shared_ptr<fw::Texture> texture);

  // Makes sure we've created all of the patches we'll need
  void ensure_patches();
public:
  terrain();
  virtual ~terrain();

  void initialize();
  void create(int width, int length, bool create_height_data = true);

  virtual void update();
  virtual void render(fw::sg::scenegraph &scenegraph);

  virtual void set_layer(int number, std::shared_ptr<fw::Bitmap> bitmap);

  // gets the height of the terrain vertex at the given integer coordinates.
  float get_vertex_height(int x, int z);

  // gets the height above the terrain at the given (x,z) coordinates.
  float get_height(float x, float z);

  inline int get_width() {
    return _width;
  }
  inline int get_length() {
    return _length;
  }

  virtual void set_splatt(int patch_x, int patch_z, fw::Bitmap const &bmp);

  // gets the collision data for the map
  std::vector<bool> const &get_collision_data() const {
    return _collision_data;
  }

  // Gets the (x,y,z) location of the point on the terrain where the cursor is pointing
  fw::Vector get_cursor_location();
  fw::Vector get_cursor_location(fw::Vector const &start,
      fw::Vector const &direction);

  // Gets the (x,y,z) of the point on the terrain that the camera is looking at
  fw::Vector get_camera_lookat();
};

}

#pragma once

#include <memory>
#include <vector>
#include <stdint.h>

#include <framework/bitmap.h>
#include <framework/math.h>
#include <framework/scenegraph.h>
#include <framework/texture.h>

namespace fw {
class VertexBuffer;
class IndexBuffer;
class Shader;
class ShaderParameters;
}

namespace ed {
class WorldWriter;
}

namespace game {

struct TerrainPatch {
  std::shared_ptr<fw::VertexBuffer> vb;
  std::shared_ptr<fw::Texture> texture;
  std::shared_ptr<fw::ShaderParameters> shader_params;

  std::shared_ptr<fw::sg::Node> node_;

  // If true, we know we need to re-bake this patch.
  bool dirty = false;
};

class Terrain {
public:
  static const int PATCH_SIZE = 64;

private:
  std::vector<std::shared_ptr<TerrainPatch>> patches_;
  std::shared_ptr<fw::IndexBuffer> ib_;
  std::shared_ptr<fw::Shader> shader_;

  // The root scenegraph node that we add all our nodes to.
  std::shared_ptr<fw::sg::Node> root_node_;

protected:
  friend class ed::WorldWriter;
  friend class WorldReader;

  std::shared_ptr<fw::TextureArray> textures_;
  std::vector<bool> collision_data_;

  const int width_;
  const int length_;
  float *heights_;

  // get the width/height of the terrain in patches
  int get_patches_width() const {
    return (width_ / PATCH_SIZE);
  }
  int get_patches_length() const {
    return (length_ / PATCH_SIZE);
  }

  // Gets the index of the given x/z coordinates for a single patch
  int get_patch_index(int patch_x, int patch_z, int *new_patch_x = 0, int *new_patch_z = 0) const;

  // bake a patch's height values into a texture, ready to be
  // passed to our vertex Shader. cool!
  void bake_patch(int patch_x, int patch_z);

  // gets or sets the splatt texture for the given patch
  std::shared_ptr<fw::Texture> get_patch_splatt(int patch_x, int patch_z);
  std::shared_ptr<fw::Texture> create_splatt(fw::Bitmap const& bmp);
  virtual void set_patch_splatt(int patch_x, int patch_z, std::shared_ptr<fw::Texture> texture);

  // Makes sure we've created all of the patches we'll need
  void ensure_patches();
public:
  Terrain(int width, int height, float *height_data = nullptr);
  virtual ~Terrain();

  virtual void initialize();
  virtual void update();

  virtual void set_layer(int number, std::shared_ptr<fw::Bitmap> bitmap);

  // gets the height of the terrain vertex at the given integer coordinates.
  float get_vertex_height(int x, int z);

  // gets the height above the terrain at the given (x,z) coordinates.
  float get_height(float x, float z);

  inline int get_width() const {
    return width_;
  }
  inline int get_length() const {
    return length_;
  }

  virtual void set_splatt(int patch_x, int patch_z, fw::Bitmap const &bmp);

  // gets the collision data for the map
  std::vector<bool> const &get_collision_data() const {
    return collision_data_;
  }

  // Gets the (x,y,z) location of the point on the terrain where the cursor is pointing
  fw::Vector get_cursor_location();
  fw::Vector get_cursor_location(fw::Vector const &start,
      fw::Vector const &direction);

  // Gets the (x,y,z) of the point on the terrain that the camera is looking at
  fw::Vector get_camera_lookat();
};

}


#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/bitmap.h>
#include <framework/texture.h>
#include <framework/exception.h>

#include <game/world/terrain_helper.h>
#include <game/editor/editor_terrain.h>

namespace ed {

static const int splatt_width = 128;
static const int splatt_height = 128;

EditorTerrain::EditorTerrain(int width, int height, float* height_data/*= nullptr*/)
  : Terrain(width, height, height_data) {
}

EditorTerrain::~EditorTerrain() {
}

// set the height of the given vertex to the given value.
void EditorTerrain::set_vertex_height(int x, int z, float height) {
  while (x < 0) {
    x += width_;
  }
  while (x >= width_) {
    x -= width_;
  }
  while (z < 0) {
    z += length_;
  }
  while (z >= length_) {
    z -= length_;
  }

  heights_[z * width_ + x] = height;

  auto this_patch = std::make_tuple(x / PATCH_SIZE, z / PATCH_SIZE);
  bool found = false;

  std::unique_lock<std::mutex> lock(patches_to_bake_mutex_);
  for(auto patch : patches_to_bake_) {
    if (patch == this_patch) {
      found = true;
      break;
    }
  }

  if (!found) {
    patches_to_bake_.push_back(this_patch);
  }
  
  if (!bake_queued_.exchange(true)) {
    fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
      [this](fw::sg::Scenegraph&) {
        std::unique_lock<std::mutex> lock(patches_to_bake_mutex_);
        for (auto patch : patches_to_bake_) {
          bake_patch(std::get<0>(patch), std::get<1>(patch));
        }
        patches_to_bake_.clear();
        bake_queued_.store(false);
      });
  }
}

void EditorTerrain::initialize_splatt() {
  std::vector<uint32_t> buffer(splatt_width * splatt_height);
  for (int y = 0; y < splatt_height; y++) {
    for (int x = 0; x < splatt_width; x++) {
      buffer[(y * splatt_width) + x] = 0x000000ff;
    }
  }

  fw::Bitmap bmp(splatt_width, splatt_height);
  bmp.set_pixels(buffer);

  ensure_patches();
  for (int z = 0; z < get_patches_length(); z++) {
    for (int x = 0; x < get_patches_width(); x++) {
      set_splatt(x, z, bmp);
    }
  }
}

void EditorTerrain::set_splatt(int patch_x, int patch_z, fw::Bitmap const &bmp) {
  std::shared_ptr<fw::Texture> splatt = get_patch_splatt(patch_x, patch_z);
  if (splatt == std::shared_ptr<fw::Texture>()) {
    splatt = std::shared_ptr<fw::Texture>(new fw::Texture());
    set_patch_splatt(patch_x, patch_z, splatt);
  }

  int index = get_patch_index(patch_x, patch_z);
  while (static_cast<int>(splatt_bitmaps_.size()) <= index) {
    // we'll add the new bitmap to all of them, but they'll eventually
    // be replaced with the correct one (well, hopefully)
    splatt_bitmaps_.push_back(bmp);
  }

  splatt_bitmaps_[index] = bmp;
  splatt->create(bmp);
}

fw::Bitmap &EditorTerrain::get_splatt(int patch_x, int patch_z) {
  int index = get_patch_index(patch_x, patch_z);
  return splatt_bitmaps_[index];
}

int EditorTerrain::get_num_layers() const {
  return layers_.size();
}

std::shared_ptr<fw::Bitmap> EditorTerrain::get_layer(int number) {
  if (number < 0 || number >= static_cast<int>(layer_bitmaps_.size()))
    return std::shared_ptr<fw::Bitmap>();

  return layer_bitmaps_[number];
}

void EditorTerrain::set_layer(int number, std::shared_ptr<fw::Bitmap> bitmap) {
  if (number < 0)
    return;

  std::shared_ptr<fw::Texture> texture(new fw::Texture());
  texture->create(bitmap);

  if (number == static_cast<int>(layers_.size())) {
    // we need to add a new layer
    layer_bitmaps_.push_back(bitmap);
    layers_.push_back(texture);
    return;
  } else if (number > static_cast<int>(layer_bitmaps_.size())) {
    // TODO: not supported yet
    return;
  }

  layer_bitmaps_[number] = bitmap;
  layers_[number] = texture;
}

void EditorTerrain::build_collision_data(std::vector<bool> &vertices) {
  if (static_cast<int>(vertices.size()) < (width_ * length_)) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("vertices vector is too small!"));
  }

  game::build_collision_data(vertices, heights_, width_, length_);
}

}

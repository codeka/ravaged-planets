
#include <boost/foreach.hpp>

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

editor_terrain::editor_terrain() {
}

editor_terrain::~editor_terrain() {
}

void editor_terrain::render(fw::sg::scenegraph &scenegraph) {
  int num_baked = 0;
  BOOST_FOREACH(auto patch, _patches_to_bake) {
    bake_patch(std::get<0>(patch), std::get<1>(patch));
    num_baked ++;
  }
  _patches_to_bake.clear();

  terrain::render(scenegraph);
}

// set the height of the given vertex to the given value.
void editor_terrain::set_vertex_height(int x, int z, float height) {
  while (x < 0) {
    x += _width;
  }
  while (x >= _width) {
    x -= _width;
  }
  while (z < 0) {
    z += _length;
  }
  while (z >= _length) {
    z -= _length;
  }

  _heights[z * _width + x] = height;

  auto this_patch = std::make_tuple(x / PATCH_SIZE, z / PATCH_SIZE);
  bool found = false;

  BOOST_FOREACH(auto patch, _patches_to_bake) {
    if (patch == this_patch) {
      found = true;
      break;
    }
  }

  if (!found) {
    _patches_to_bake.push_back(this_patch);
  }
}

void editor_terrain::initialize_splatt() {
  std::vector<uint32_t> buffer(splatt_width * splatt_height);
  for (int y = 0; y < splatt_height; y++) {
    for (int x = 0; x < splatt_width; x++) {
      buffer[(y * splatt_width) + x] = 0x000000ff;
    }
  }

  fw::bitmap bmp(splatt_width, splatt_height);
  bmp.set_pixels(buffer);

  ensure_patches();
  for (int z = 0; z < get_patches_length(); z++) {
    for (int x = 0; x < get_patches_width(); x++) {
      set_splatt(x, z, bmp);
    }
  }
}

void editor_terrain::set_splatt(int patch_x, int patch_z, fw::bitmap const &bmp) {
  std::shared_ptr<fw::texture> splatt = get_patch_splatt(patch_x, patch_z);
  if (splatt == std::shared_ptr<fw::texture>()) {
    splatt = std::shared_ptr<fw::texture>(new fw::texture());
    set_patch_splatt(patch_x, patch_z, splatt);
  }

  int index = get_patch_index(patch_x, patch_z);
  while (static_cast<int>(_splatt_bitmaps.size()) <= index) {
    // we'll add the new bitmap to all of them, but they'll eventually
    // be replaced with the correct one (well, hopefully)
    _splatt_bitmaps.push_back(bmp);
  }

  _splatt_bitmaps[index] = bmp;
  splatt->create(bmp);
}

fw::bitmap &editor_terrain::get_splatt(int patch_x, int patch_z) {
  int index = get_patch_index(patch_x, patch_z);
  return _splatt_bitmaps[index];
}

std::shared_ptr<fw::texture> editor_terrain::get_layer(int number) {
  if (number < 0 || number >= static_cast<int>(_layers.size()))
    return std::shared_ptr<fw::texture>();

  return _layers[number];
}

void editor_terrain::set_layer(int number, std::shared_ptr<fw::texture> texture) {
  if (number < 0)
    return;

  if (number == static_cast<int>(_layers.size())) {
    // we need to add a new layer
    _layers.push_back(texture);
  } else if (number > static_cast<int>(_layers.size())) {
    return;
  }

  _layers[number] = texture;
}

void editor_terrain::build_collision_data(std::vector<bool> &vertices) {
  if (static_cast<int>(vertices.size()) < (_width * _length)) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("vertices vector is too small!"));
  }

  game::build_collision_data(vertices, _heights, _width, _length);
}

}

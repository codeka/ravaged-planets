
#include <framework/misc.h>
#include <framework/exception.h>
#include <framework/graphics.h>

#include <game/world/terrain_helper.h>

namespace game {

void generate_terrain_indices(std::vector<uint16_t> &indices, int patch_size) {
  patch_size++;

  int num_indices = (patch_size * 2) * (patch_size - 1) + (patch_size - 2);
  indices.resize(num_indices);

  int index = 0;
  for (int z = 0; z < patch_size - 1; z++) {
    // even rows move left to right, odd rows move right to left.
    if (z % 2 == 0) {
      int x;
      for (x = 0; x < patch_size; x++) {
        indices[index++] = x + (z * patch_size);
        indices[index++] = x + (z * patch_size) + patch_size;
      }

      // insert degenerate vertex if this isn't the last row
      if (z != patch_size - 2) {
        x--;
        indices[index++] = x + (z * patch_size);
      }
    } else {
      int x;
      for (x = patch_size - 1; x >= 0; x--) {
        indices[index++] = x + (z * patch_size);
        indices[index++] = x + (z * patch_size) + patch_size;
      }
      // insert degenerate vertex if this isn't the last row
      if (z != patch_size - 2) {
        x++;
        indices[index++] = x + (z * patch_size);
      }
    }
  }
}

void generate_terrain_indices_wireframe(std::vector<uint16_t> &indices, int patch_size) {
  int num_indices = patch_size * patch_size * 4;
  indices.resize(num_indices);

  int index = 0;
  for (int z = 0; z < patch_size; z++) {
    for (int x = 0; x < patch_size; x++) {
      indices[index++] = (z * (patch_size + 1)) + x;
      indices[index++] = (z * (patch_size + 1)) + (x + 1);
      indices[index++] = (z * (patch_size + 1)) + x;
      indices[index++] = ((z + 1) * (patch_size + 1)) + x;
    }
  }
}

// gets the height of a vertex at the given (x,z) location, using fw::constrain() to
// constrain the coordinates to the width/length.
fw::Vector get_vertex(int x, int z, float *height, int width, int length) {
  int ix = fw::constrain(x, width);
  int iz = fw::constrain(z, length);

  return fw::Vector((float) x, height[iz * width + ix], (float) z);
}

// calculates the normal at the given location in the map
fw::Vector calculate_normal(float *heights, int width, int length, int x, int z) {
  fw::Vector center = get_vertex(x, z, heights, width, length);
  fw::Vector north = get_vertex(x, z + 1, heights, width, length);
  fw::Vector south = get_vertex(x, z - 1, heights, width, length);
  fw::Vector east = get_vertex(x + 1, z, heights, width, length);
  fw::Vector west = get_vertex(x - 1, z, heights, width, length);

  fw::Vector a = north - center;
  fw::Vector b = east - center;
  fw::Vector c = south - center;
  fw::Vector d = west - center;

  fw::Vector normal(0, 0, 0);
  normal += cml::cross(a, b);
  normal += cml::cross(b, c);
  normal += cml::cross(c, d);
  normal += cml::cross(d, a);
  return normal.normalize();
}

int generate_terrain_vertices(fw::vertex::xyz_n **buffer, float *height, int width, int length,
    int patch_size /* = 0 */, int patch_x /* = 0 */, int patch_z /* = 0 */) {
  if (patch_size == 0) {
    if (width != length) {
      BOOST_THROW_EXCEPTION(
          fw::Exception()
              << fw::message_error_info(
                  "If you don't specify a patch_size, width and height must be the same"));
    }

    patch_size = width;
  }

  (*buffer) = new fw::vertex::xyz_n[(patch_size + 1) * (patch_size + 1)];
  for (int z = 0; z <= patch_size; z++) {
    for (int x = 0; x <= patch_size; x++) {
      int ix = (patch_x * patch_size) + x;
      int iz = (patch_z * patch_size) + z;
      fw::Vector center = get_vertex(ix, iz, height, width, length);
      fw::Vector normal = calculate_normal(height, width, length, ix, iz);

      int verts_index = z * (patch_size + 1) + x;
      (*buffer)[verts_index] = fw::vertex::xyz_n(x, center[1], z, normal[0],
          normal[1], normal[2]);
    }
  }

  return (patch_size + 1) * (patch_size + 1);
}

void build_collision_data(std::vector<bool> &vertices, float *heights,  int width, int length) {
  fw::Vector up(0, 1, 0);

  for (int z = 0; z < length; z++) {
    for (int x = 0; x < width; x++) {
      fw::Vector normal = calculate_normal(heights, width, length, x, z);
      float dot = cml::dot(up, normal);

      // todo: we should store the actual dot product, since it could be useful in other places.
      vertices[x + (z * width)] = (dot > 0.85f);
    }
  }
}

}

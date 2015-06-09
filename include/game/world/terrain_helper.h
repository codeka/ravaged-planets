#pragma once

namespace fw {
namespace vertex {
struct xyz_n;
}
}

namespace rp {

// generates a "patch" of indices for a terrain that's patch_size * patch_size big
void generate_terrain_indices(std::vector<uint16_t> &indices, int patch_size);

// similar to generate_terrain_indices, but we generate indices assuming a wire
// mesh will be drawn
void generate_terrain_indices_wireframe(std::vector<uint16_t> &indices, int patch_size);

// generates xyz_n vertices for a patch of terrain the of given size.
//
// patch_x, patch_y is the patch offset into the given height data that we want to
// generate data for
// height is a pointer to the actual height data we're going to generate the terrain for
// width, height is the width/height of the total terrain data
int generate_terrain_vertices(fw::vertex::xyz_n **buffer, float *height,
    int width, int length, int patch_size = 0, int patch_x = 0,
    int patch_z = 0);

// see editor_terrain::build_collision_data, which we're based off of
void build_collision_data(std::vector<bool> &vertices, float *heights,
    int width, int length);

}


#include <framework/graphics.h>
#include <framework/misc.h>
#include <framework/paths.h>
#include <framework/logging.h>
#include <framework/camera.h>
#include <framework/bitmap.h>
#include <framework/texture.h>
#include <framework/input.h>
#include <framework/vector.h>
#include <framework/scenegraph.h>
#include <framework/shader.h>
#include <game/world/terrain.h>
#include <game/world/terrain_helper.h>

namespace game {

terrain::terrain() :
    _width(0), _length(0), _heights(nullptr) {
}

terrain::~terrain() {
  delete[] _heights;
}

void terrain::initialize() {
  // generate indices
  std::vector<uint16_t> index_data;
  generate_terrain_indices(index_data, PATCH_SIZE);
  _ib = std::shared_ptr<fw::index_buffer>(new fw::index_buffer());
  _ib->set_data(index_data.size(), &index_data[0], 0);

  // load the shader file that we'll use for rendering
  _shader = fw::shader::create("terrain.shader");

  // TODO: this should come from the world_reader
  set_layer(0, std::make_shared<fw::Bitmap>(fw::resolve("terrain/grass-01.jpg")));
  set_layer(1, std::make_shared<fw::Bitmap>(fw::resolve("terrain/rock-01.jpg")));
  set_layer(2, std::make_shared<fw::Bitmap>(fw::resolve("terrain/snow-01.jpg")));
  set_layer(3, std::make_shared<fw::Bitmap>(fw::resolve("terrain/rock-snow-01.jpg")));

  // bake the patches into the vertex buffers that'll be used for rendering
  for (int patch_z = 0; patch_z < get_patches_length(); patch_z++) {
    for (int patch_x = 0; patch_x < get_patches_width(); patch_x++) {
      bake_patch(patch_x, patch_z);
    }
  }
}

void terrain::create(int width, int length, bool create_height_data /*= true */) {
  _width = width;
  _length = length;

  if (create_height_data) {
    _heights = new float[_width * _length];
    for (int i = 0; i < _width * _length; i++)
      _heights[i] = 0.0f;
  } else {
    _heights = nullptr;
  }
}

std::shared_ptr<fw::texture> terrain::get_patch_splatt(int patch_x, int patch_z) {
  ensure_patches();

  unsigned int index = get_patch_index(patch_x, patch_z);
  return _patches[index]->texture;
}

void terrain::set_layer(int number, std::shared_ptr<fw::Bitmap> bitmap) {
  if (number < 0)
    return;

  std::shared_ptr<fw::texture> texture(new fw::texture());
  texture->create(bitmap);

  if (number == static_cast<int>(_layers.size())) {
    // we need to add a new layer
    _layers.push_back(texture);
    return;
  } else if (number > static_cast<int>(_layers.size())) {
    // TODO: not supported yet
    return;
  }

  _layers[number] = texture;
}

void terrain::set_patch_splatt(int patch_x, int patch_z, std::shared_ptr<fw::texture> texture) {
  ensure_patches();

  unsigned int index = get_patch_index(patch_x, patch_z);
  _patches[index]->texture = texture;
}

void terrain::set_splatt(int patch_x, int patch_z, fw::Bitmap const &bmp) {
  std::shared_ptr<fw::texture> splatt(new fw::texture());
  splatt->create(bmp);

  set_patch_splatt(patch_x, patch_z, splatt);
}

void terrain::bake_patch(int patch_x, int patch_z) {
  unsigned int index = get_patch_index(patch_x, patch_z, &patch_x, &patch_z);
  ensure_patches();

  // if we haven't created the vertex buffer for this patch yet, do it now
  if (_patches[index]->vb == std::shared_ptr<fw::vertex_buffer>()) {
    _patches[index]->vb = fw::vertex_buffer::create<fw::vertex::xyz_n>();
  }

  fw::vertex::xyz_n *vert_data;
  int num_verts = generate_terrain_vertices(&vert_data, _heights, _width, _length, PATCH_SIZE, patch_x, patch_z);

  std::shared_ptr<terrain_patch> patch(_patches[index]);
  patch->vb->set_data(num_verts, vert_data, 0);
  delete[] vert_data;

  patch->shader_params = _shader->create_parameters();
  if (_layers.size() >= 1)
    patch->shader_params->set_texture("layer1", _layers[0]);
  if (_layers.size() >= 2)
    patch->shader_params->set_texture("layer2", _layers[1]);
  if (_layers.size() >= 3)
    patch->shader_params->set_texture("layer3", _layers[2]);
  if (_layers.size() >= 4)
    patch->shader_params->set_texture("layer4", _layers[3]);
  patch->shader_params->set_texture("splatt", patch->texture);
}

void terrain::ensure_patches() {
  unsigned int index = get_patch_index(get_patches_width() - 1,
      get_patches_length() - 1);
  while (_patches.size() <= index) {
    std::shared_ptr<terrain_patch> patch(new terrain_patch());
    _patches.push_back(patch);
  }
}

void terrain::update() {
  fw::camera *camera = fw::framework::get_instance()->get_camera();

  // if the camera has moved off the edge of the map, wrap it back around
  fw::vector old_loc = camera->get_location();
  fw::vector new_loc(
      fw::constrain(old_loc[0], (float) _width),
      old_loc[1],
      fw::constrain(old_loc[2], (float) _length));
  if ((old_loc - new_loc).length_squared() > 0.001f) {
    camera->set_location(new_loc);
  }

  // also, set the ground height so the camera follows the terrain
  float new_height = get_height(new_loc[0], new_loc[2]);
  float height_diff = camera->get_ground_height() - new_height;
  if (height_diff > 0.001f || height_diff < -0.001f) {
    camera->set_ground_height(new_height);
  }
}

void terrain::render(fw::sg::scenegraph &scenegraph) {
  if (_layers.size() == 0)
    return;

  // we want to render the terrain centered on where the camera is looking
  fw::camera *camera = fw::framework::get_instance()->get_camera();
  fw::vector location = get_cursor_location(camera->get_position(), camera->get_direction());

  int centre_patch_x = (int) (location[0] / PATCH_SIZE);
  int centre_patch_z = (int) (location[2] / PATCH_SIZE);

  for (int patch_z = centre_patch_z - 1; patch_z <= centre_patch_z + 1; patch_z++) {
    for (int patch_x = centre_patch_x - 1; patch_x <= centre_patch_x + 1; patch_x++) {
      int patch_index = get_patch_index(patch_x, patch_z);

      std::shared_ptr<terrain_patch> patch(_patches[patch_index]);

      // set up the world matrix for this patch so that it's being rendered at the right offset
      std::shared_ptr<fw::sg::node> node(new fw::sg::node());
      fw::matrix world = fw::translation(
          static_cast<float>(patch_x * PATCH_SIZE), 0,
          static_cast<float>(patch_z * PATCH_SIZE));
      node->set_world_matrix(world);

      // we have to set up the scenegraph node with these manually
      node->set_vertex_buffer(patch->vb);
      node->set_index_buffer(_ib);
      node->set_shader(_shader);
      node->set_shader_parameters(patch->shader_params);
      node->set_primitive_type(fw::sg::primitive_trianglestrip);

      scenegraph.add_node(node);
    }
  }
}

int terrain::get_patch_index(int patch_x, int patch_z, int *new_patch_x, int *new_patch_z) {
  patch_x = fw::constrain(patch_x, get_patches_width());
  patch_z = fw::constrain(patch_z, get_patches_length());

  if (new_patch_x != 0)
    *new_patch_x = patch_x;
  if (new_patch_z != 0)
    *new_patch_z = patch_z;

  return patch_z * get_patches_width() + patch_x;
}

float terrain::get_vertex_height(int x, int z) {
  return _heights[fw::constrain(z, _length) * _width + fw::constrain(x, _width)];
}

//
// this method is pretty simple. we basically get the height at (x, z) then interpolate that
// value between (x+1, z+1).
//
float terrain::get_height(float x, float z) {
  int x0 = (int) floor(x);
  int x1 = x0 + 1;

  int z0 = (int) floor(z);
  int z1 = z0 + 1;

  float h00 = get_vertex_height(x0, z0);
  float h10 = get_vertex_height(x1, z0);
  ;
  float h01 = get_vertex_height(x0, z1);
  float h11 = get_vertex_height(x1, z1);

  float dx = x - x0;
  float dz = z - z0;
  float dxdz = dx * dz;

  return h00 * (1.0f - dz - dx + dxdz) + h10 * (dx - dxdz) + h11 * dxdz + h01 * (dz - dxdz);
}

// gets the point on the terrain that the camera is currently looking at
fw::vector terrain::get_camera_lookat() {
  fw::framework *frmwrk = fw::framework::get_instance();
  fw::camera *camera = frmwrk->get_camera();

  fw::vector centre = camera->unproject(0.0f, 0.0f);
  fw::vector start = camera->get_position();
  fw::vector direction = (centre - start).normalize();

  return get_cursor_location(start, direction);
}

// this method is fairly simple, we just trace a line from the
// camera through the cursor point to the end of the terrain, projected
// in the (x,y) plane (so it's a nice, easy 2D line). Then, for each
// 2D point on that line, we check how close the ray is to that point
// in 3D-space. If it's close enough, we return that one...
fw::vector terrain::get_cursor_location() {
  fw::framework *frmwrk = fw::framework::get_instance();
  fw::input *input = frmwrk->get_input();
  fw::camera *camera = frmwrk->get_camera();

  float mx = (float) input->mouse_x();
  float my = (float) input->mouse_y();

  mx = 1.0f - (2.0f * mx / frmwrk->get_graphics()->get_width());
  my = 1.0f - (2.0f * my / frmwrk->get_graphics()->get_height());

  fw::vector mvec = camera->unproject(-mx, my);

  fw::vector start = camera->get_position();
  fw::vector direction = (mvec - start).normalize();

  return get_cursor_location(start, direction);
}

fw::vector terrain::get_cursor_location(fw::vector const &start, fw::vector const &direction) {
  fw::vector evec = start + (direction * 150.0f);
  fw::vector svec = start + (direction * 5.0f);

  // todo: we use the same algorithm here and in path_find::is_passable(fw::vector const &start, fw::vector const &end)
  // can we factor it out?
  int sx = static_cast<int>(floor(svec[0] + 0.5f));
  int sz = static_cast<int>(floor(svec[2] + 0.5f));
  int ex = static_cast<int>(floor(evec[0] + 0.5f));
  int ez = static_cast<int>(floor(evec[2] + 0.5f));

  int dx = ex - sx;
  int dz = ez - sz;
  int abs_dx = abs(dx);
  int abs_dz = abs(dz);

  int steps = (abs_dx > abs_dz) ? abs_dx : abs_dz;

  float xinc = dx / static_cast<float>(steps);
  float zinc = dz / static_cast<float>(steps);

  float x = static_cast<float>(sx);
  float z = static_cast<float>(sz);
  for (int i = 0; i <= steps; i++) {
    // we actually check in a 9x9 matrix around this point, to make sure
    // we check all possible candidates. Because we return as soon as
    // match is found, this shouldn't be too bad...
    int ix = static_cast<int>(floor(x + 0.5f));
    int iz = static_cast<int>(floor(z + 0.5f));
    for (int oz = iz - 1; oz <= iz + 1; oz++) {
      for (int ox = ix - 1; ox <= ix + 1; ox++) {
        float x1 = static_cast<float>(ox);
        float x2 = static_cast<float>(ox + 1);
        float z1 = static_cast<float>(oz);
        float z2 = static_cast<float>(oz + 1);

        fw::vector p11(x1, get_vertex_height(static_cast<int>(x1), static_cast<int>(z1)), z1);
        fw::vector p21(x2, get_vertex_height(static_cast<int>(x2), static_cast<int>(z1)), z1);
        fw::vector p12(x1, get_vertex_height(static_cast<int>(x1), static_cast<int>(z2)), z2);
        fw::vector p22(x2, get_vertex_height(static_cast<int>(x2), static_cast<int>(z2)), z2);

        fw::vector n1 = cml::cross(p12 - p11, p21 - p11);
        fw::vector n2 = cml::cross(p21 - p22, p12 - p22);

        // the line intersects the plane defined by the first triangle at i1. If that's
        // within this triangle's "x,z" coordinates, this is the intersection point!
        fw::vector i1 = fw::point_plane_intersect(p11, n1, start, direction);
        if (i1[0] > x1 && i1[0] <= x2 && i1[2] > z1 && i1[2] <= z2) {
          return i1;
        }

        // same calculation for the second triangle
        fw::vector i2 = fw::point_plane_intersect(p22, n2, start, direction);
        if (i2[0] > x1 && i2[0] <= x2 && i2[2] > z1 && i2[2] <= z2) {
          return i2;
        }
      }
    }

    x += xinc;
    z += zinc;
  }

  return fw::vector(0, 0, 0);
}
}

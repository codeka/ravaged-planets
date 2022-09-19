
#include <boost/foreach.hpp>

#include <framework/particle_renderer.h>
#include <framework/particle_manager.h>
#include <framework/particle.h>
#include <framework/particle_config.h>
#include <framework/graphics.h>
#include <framework/shader.h>
#include <framework/texture.h>
#include <framework/camera.h>
#include <framework/logging.h>
#include <framework/paths.h>
#include <framework/vector.h>
#include <framework/misc.h>
#include <framework/scenegraph.h>
#include <framework/exception.h>

//-----------------------------------------------------------------------------
// This structure is used to sort particles first by texture and then by z-order (to avoid state changes and
// ordering issues)
struct particle_sorter {
  fw::Vector _cam_pos;

  particle_sorter(fw::Vector camera_pos) :
      _cam_pos(camera_pos) {
  }

  bool operator()(fw::particle const *lhs, fw::particle const *rhs) {
    if (lhs->config->billboard.texture != rhs->config->billboard.texture) {
      // it doesn't matter which order we choose for the textures,
      // as long TextureA always appears on the "same side" of TextureB.
      return (lhs->config->billboard.texture.get() > rhs->config->billboard.texture.get());
    }

    if (lhs->config->billboard.mode != rhs->config->billboard.mode) {
      return (lhs->config->billboard.mode > rhs->config->billboard.mode);
    }

    float lhs_len = (lhs->pos - _cam_pos).length();
    float rhs_len = (rhs->pos - _cam_pos).length();
    return lhs_len > rhs_len;
  }
};

//-----------------------------------------------------------------------------

const int batch_size = 1024;
const int max_vertices = batch_size * 4;
const int max_indices = batch_size * 6;

struct render_state {
  fw::sg::scenegraph &scenegraph;
  int particle_num;
  std::vector<fw::vertex::xyz_c_uv> vertices;
  std::vector<uint16_t> indices;
  std::shared_ptr<fw::Texture> texture;
  fw::particle_emitter_config::billboard_mode mode;
  fw::particle_renderer::particle_list &particles;
  std::shared_ptr<fw::shader> shader;
  std::shared_ptr<fw::shader_parameters> shader_parameters;

  std::vector<std::shared_ptr<fw::vertex_buffer>> vertex_buffers;
  std::vector<std::shared_ptr<fw::index_buffer>> index_buffers;

  inline render_state(fw::sg::scenegraph &sg, fw::particle_renderer::particle_list &particles) :
      scenegraph(sg), particles(particles), mode(fw::particle_emitter_config::additive), particle_num(0) {
  }
};

//-----------------------------------------------------------------------------
// This class holds the index and vertex buffer(s) while we're not using them...
// so that we don't have to continually create/destroy them
class buffer_cache {
private:
  std::vector<std::shared_ptr<fw::vertex_buffer>> _vertex_buffers;
  std::vector<std::shared_ptr<fw::index_buffer>> _index_buffers;

public:
  std::shared_ptr<fw::vertex_buffer> get_vertex_buffer();
  std::shared_ptr<fw::index_buffer> get_index_buffer();

  void release_vertex_buffer(std::shared_ptr<fw::vertex_buffer> vb);
  void release_index_buffer(std::shared_ptr<fw::index_buffer> ib);
};

std::shared_ptr<fw::vertex_buffer> buffer_cache::get_vertex_buffer() {
  if (_vertex_buffers.size() > 0) {
    std::shared_ptr<fw::vertex_buffer> vb(_vertex_buffers.back());
    _vertex_buffers.pop_back();
    return vb;
  } else {
    return fw::vertex_buffer::create<fw::vertex::xyz_c_uv>(true);
  }
}

std::shared_ptr<fw::index_buffer> buffer_cache::get_index_buffer() {
  if (_index_buffers.size() > 0) {
    std::shared_ptr<fw::index_buffer> ib(_index_buffers.back());
    _index_buffers.pop_back();
    return ib;
  } else {
    return std::shared_ptr<fw::index_buffer>(new fw::index_buffer(true));
  }
}

void buffer_cache::release_vertex_buffer(std::shared_ptr<fw::vertex_buffer> vb) {
  _vertex_buffers.push_back(vb);
}

void buffer_cache::release_index_buffer(std::shared_ptr<fw::index_buffer> ib) {
  _index_buffers.push_back(ib);
}

static buffer_cache g_buffer_cache;

//-----------------------------------------------------------------------------

namespace fw {

particle_renderer::particle_renderer(particle_manager *mgr) :
    _graphics(nullptr), _shader(nullptr), _mgr(mgr), _draw_frame(1), _colour_texture(new fw::Texture()) {
}

particle_renderer::~particle_renderer() {
}

void particle_renderer::initialize(graphics *g) {
  _graphics = g;

  _colour_texture->create(fw::resolve("particles/colours.png"));
  _shader = fw::shader::create("particle.shader");
  _shader_params = _shader->create_parameters();
  _shader_params->set_texture("colour_texture", _colour_texture);
}

/** Gets the name of the program in the particle.shader file we'll use for the given billboard_mode. */
std::string get_program_name(particle_emitter_config::billboard_mode mode) {
  switch (mode) {
  case particle_emitter_config::normal:
    return "particle-normal";
  case particle_emitter_config::additive:
    return "particle-additive";
  default:
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Unknown billboard_mode!"));

    return ""; // never gets here...
  }
}

void generate_scenegraph_node(render_state &rs) {
  std::shared_ptr<fw::vertex_buffer> vb = g_buffer_cache.get_vertex_buffer();
  vb->set_data(rs.vertices.size(), &rs.vertices[0]);
  rs.vertices.clear();
  rs.vertex_buffers.push_back(vb);

  std::shared_ptr<fw::index_buffer> ib = g_buffer_cache.get_index_buffer();
  ib->set_data(rs.indices.size(), &rs.indices[0]);
  rs.indices.clear();
  rs.index_buffers.push_back(ib);

  std::shared_ptr<fw::shader_parameters> shader_params = rs.shader_parameters->clone();
  shader_params->set_program_name(get_program_name(rs.mode));
  shader_params->set_texture("particle_texture", rs.texture);

  std::shared_ptr<sg::node> node(new sg::node());
  node->set_vertex_buffer(vb);
  node->set_index_buffer(ib);
  node->set_shader(rs.shader);
  node->set_shader_parameters(shader_params);
  node->set_primitive_type(fw::sg::primitive_trianglelist);
  node->set_cast_shadows(false);
  rs.scenegraph.add_node(node);
}

bool particle_renderer::add_particle(render_state &rs, int base_index, particle *p, float offset_x, float offset_z) {
  fw::Camera *cam = fw::framework::get_instance()->get_camera();
  fw::Vector pos(p->pos[0] + offset_x, p->pos[1], p->pos[2] + offset_z);

  // only render if the (absolute, not wrapped) distance to the camera is < 50
  fw::Vector dir_to_cam = (cam->get_position() - pos);
  if (dir_to_cam.length_squared() > (50.0f * 50.0f))
    return false;

  // if we've already drawn the particle this frame, don't do it again.
  if (p->draw_frame == _draw_frame)
    return false;
  p->draw_frame = _draw_frame;

  // the colour_row is divided by this value to get the value between 0 and 1.
  float colour_texture_factor = 1.0f / this->_colour_texture->get_height();

  fw::colour colour(p->alpha, (static_cast<float>(p->colour1) + 0.5f) * colour_texture_factor,
      (static_cast<float>(p->colour2) + 0.5f) * colour_texture_factor, p->colour_factor);

  Matrix m = fw::scale(p->size);
  if (p->rotation_kind != rotation_kind::direction) {
    m *= fw::rotate_axis_angle(Vector(0, 0, 1), p->angle);

    Matrix m2;
    cml::matrix_rotation_align(m2, dir_to_cam);
    m *= m2;
  } else {
    Matrix m2;
    cml::matrix_rotation_vec_to_vec(m2, Vector(-1, 0, 0), p->direction);
    m *= m2;
  }
  m *= fw::translation(pos);

  float aspect = (p->rect.bottom - p->rect.top) / (p->rect.right - p->rect.left);

  fw::Vector v = cml::transform_point(m, fw::Vector(-0.5f, -0.5f * aspect, 0));
  rs.vertices.push_back(fw::vertex::xyz_c_uv(v[0], v[1], v[2], colour.to_abgr(), p->rect.left, p->rect.bottom));
  v = cml::transform_point(m, fw::Vector(-0.5f, 0.5f * aspect, 0));
  rs.vertices.push_back(fw::vertex::xyz_c_uv(v[0], v[1], v[2], colour.to_abgr(), p->rect.left, p->rect.top));
  v = cml::transform_point(m, fw::Vector(0.5f, 0.5f * aspect, 0));
  rs.vertices.push_back(fw::vertex::xyz_c_uv(v[0], v[1], v[2], colour.to_abgr(), p->rect.right, p->rect.top));
  v = cml::transform_point(m, fw::Vector(0.5f, -0.5f * aspect, 0));
  rs.vertices.push_back(fw::vertex::xyz_c_uv(v[0], v[1], v[2], colour.to_abgr(), p->rect.right, p->rect.bottom));

  rs.indices.push_back(base_index);
  rs.indices.push_back(base_index + 1);
  rs.indices.push_back(base_index + 2);
  rs.indices.push_back(base_index);
  rs.indices.push_back(base_index + 2);
  rs.indices.push_back(base_index + 3);

  return true;
}

void particle_renderer::render_particles(render_state &rs, float offset_x, float offset_z) {
  for (particle_renderer::particle_list::iterator it = rs.particles.begin(); it != rs.particles.end(); ++it) {
    particle *p = *it;

    if (rs.texture != p->config->billboard.texture || rs.particle_num >= batch_size
        || rs.mode != p->config->billboard.mode) {
      if (rs.texture && rs.particle_num > 0) {
        generate_scenegraph_node(rs);
      }

      rs.particle_num = 0;
      rs.texture = p->config->billboard.texture;
      rs.mode = p->config->billboard.mode;
    }

    int base_index = rs.particle_num * 4;
    if (add_particle(rs, base_index, p, offset_x, offset_z))
      rs.particle_num++;
  }
}

void particle_renderer::render(sg::scenegraph &scenegraph, particle_renderer::particle_list &particles) {
  if (particles.size() == 0)
    return;

  // make sure the particle's pos is update
  BOOST_FOREACH(particle *p, particles) {
    p->pos = p->new_pos;
  }

  // sort the particles by texture, then by z-order
  sort_particles(particles);

  // create the render state that'll hold all our state variables
  render_state rs(scenegraph, particles);
  rs.shader = _shader;
  rs.shader_parameters = _shader_params;
  rs.particle_num = 0;
  rs.mode = particle_emitter_config::normal;

  if (_mgr->get_wrap_x() > 1.0f && _mgr->get_wrap_z() > 1.0f) {
    for (int z = -1; z <= 1; z++) {
      for (int x = -1; x <= 1; x++) {
        float offset_x = x * _mgr->get_wrap_x();
        float offset_z = z * _mgr->get_wrap_z();
        render_particles(rs, offset_x, offset_z);
      }
    }
  } else {
    render_particles(rs, 0, 0);
  }

  if (rs.particle_num > 0) {
    generate_scenegraph_node(rs);
  }

  // release the vertex and index buffers back into the cache (even though
  // they're still technically in use by the scene graph, we won't need them
  // again until the next frame so we can do this here)
  BOOST_FOREACH(std::shared_ptr<fw::vertex_buffer> vb, rs.vertex_buffers) {
    g_buffer_cache.release_vertex_buffer(vb);
  }
  BOOST_FOREACH(std::shared_ptr<fw::index_buffer> ib, rs.index_buffers) {
    g_buffer_cache.release_index_buffer(ib);
  }

  // now this frame is over record it so we'll continue to draw particles next frame
  _draw_frame++;
}

void particle_renderer::sort_particles(particle_renderer::particle_list &particles) {
  fw::Camera *cam = fw::framework::get_instance()->get_camera();
  fw::Vector const &cam_pos = cam->get_position();

  particles.sort(particle_sorter(cam_pos));
}

}

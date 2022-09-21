
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
struct ParticleSorter {
  fw::Vector cam_pos_;

  ParticleSorter(fw::Vector camera_pos) :
      cam_pos_(camera_pos) {
  }

  bool operator()(fw::Particle const *lhs, fw::Particle const *rhs) {
    if (lhs->config->billboard.texture != rhs->config->billboard.texture) {
      // it doesn't matter which order we choose for the textures,
      // as long TextureA always appears on the "same side" of TextureB.
      return (lhs->config->billboard.texture.get() > rhs->config->billboard.texture.get());
    }

    if (lhs->config->billboard.mode != rhs->config->billboard.mode) {
      return (lhs->config->billboard.mode > rhs->config->billboard.mode);
    }

    float lhs_len = (lhs->pos - cam_pos_).length();
    float rhs_len = (rhs->pos - cam_pos_).length();
    return lhs_len > rhs_len;
  }
};

//-----------------------------------------------------------------------------

const int batch_size = 1024;
const int max_vertices = batch_size * 4;
const int max_indices = batch_size * 6;

struct RenderState {
  fw::sg::Scenegraph &Scenegraph;
  int particle_num;
  std::vector<fw::vertex::xyz_c_uv> vertices;
  std::vector<uint16_t> indices;
  std::shared_ptr<fw::Texture> texture;
  fw::ParticleEmitterConfig::BillboardMode mode;
  fw::ParticleRenderer::ParticleList &particles;
  std::shared_ptr<fw::Shader> Shader;
  std::shared_ptr<fw::ShaderParameters> ShaderParameters;

  std::vector<std::shared_ptr<fw::VertexBuffer>> vertex_buffers;
  std::vector<std::shared_ptr<fw::IndexBuffer>> index_buffers;

  inline RenderState(fw::sg::Scenegraph &sg, fw::ParticleRenderer::ParticleList &particles) :
      Scenegraph(sg), particles(particles), mode(fw::ParticleEmitterConfig::kAdditive), particle_num(0) {
  }
};

//-----------------------------------------------------------------------------
// This class holds the index and vertex buffer(s) while we're not using them...
// so that we don't have to continually create/destroy them
class BufferCache {
private:
  std::vector<std::shared_ptr<fw::VertexBuffer>> vertex_buffers_;
  std::vector<std::shared_ptr<fw::IndexBuffer>> index_buffers_;

public:
  std::shared_ptr<fw::VertexBuffer> get_vertex_buffer();
  std::shared_ptr<fw::IndexBuffer> get_index_buffer();

  void release_vertex_buffer(std::shared_ptr<fw::VertexBuffer> vb);
  void release_index_buffer(std::shared_ptr<fw::IndexBuffer> ib);
};

std::shared_ptr<fw::VertexBuffer> BufferCache::get_vertex_buffer() {
  if (vertex_buffers_.size() > 0) {
    std::shared_ptr<fw::VertexBuffer> vb(vertex_buffers_.back());
    vertex_buffers_.pop_back();
    return vb;
  } else {
    return fw::VertexBuffer::create<fw::vertex::xyz_c_uv>(true);
  }
}

std::shared_ptr<fw::IndexBuffer> BufferCache::get_index_buffer() {
  if (index_buffers_.size() > 0) {
    std::shared_ptr<fw::IndexBuffer> ib(index_buffers_.back());
    index_buffers_.pop_back();
    return ib;
  } else {
    return std::shared_ptr<fw::IndexBuffer>(new fw::IndexBuffer(true));
  }
}

void BufferCache::release_vertex_buffer(std::shared_ptr<fw::VertexBuffer> vb) {
  vertex_buffers_.push_back(vb);
}

void BufferCache::release_index_buffer(std::shared_ptr<fw::IndexBuffer> ib) {
  index_buffers_.push_back(ib);
}

static BufferCache g_buffer_cache;

//-----------------------------------------------------------------------------

namespace fw {

ParticleRenderer::ParticleRenderer(ParticleManager *mgr) :
    graphics_(nullptr), shader_(nullptr), mgr_(mgr), _draw_frame(1), color_texture_(new fw::Texture()) {
}

ParticleRenderer::~ParticleRenderer() {
}

void ParticleRenderer::initialize(Graphics *g) {
  graphics_ = g;

  color_texture_->create(fw::resolve("particles/colors.png"));
  shader_ = fw::Shader::create("particle.shader");
  shader_params_ = shader_->create_parameters();
  shader_params_->set_texture("color_texture", color_texture_);
}

/** Gets the name of the program in the Particle.Shader file we'll use for the given BillboardMode. */
std::string get_program_name(ParticleEmitterConfig::BillboardMode mode) {
  switch (mode) {
  case ParticleEmitterConfig::kNormal:
    return "particle-normal";
  case ParticleEmitterConfig::kAdditive:
    return "particle-additive";
  default:
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("Unknown billboard_mode!"));

    return ""; // never gets here...
  }
}

void generate_scenegraph_node(RenderState &rs) {
  std::shared_ptr<fw::VertexBuffer> vb = g_buffer_cache.get_vertex_buffer();
  vb->set_data(rs.vertices.size(), &rs.vertices[0]);
  rs.vertices.clear();
  rs.vertex_buffers.push_back(vb);

  std::shared_ptr<fw::IndexBuffer> ib = g_buffer_cache.get_index_buffer();
  ib->set_data(rs.indices.size(), &rs.indices[0]);
  rs.indices.clear();
  rs.index_buffers.push_back(ib);

  std::shared_ptr<fw::ShaderParameters> shader_params = rs.ShaderParameters->clone();
  shader_params->set_program_name(get_program_name(rs.mode));
  shader_params->set_texture("particle_texture", rs.texture);

  std::shared_ptr<sg::Node> Node(new sg::Node());
  Node->set_vertex_buffer(vb);
  Node->set_index_buffer(ib);
  Node->set_shader(rs.Shader);
  Node->set_shader_parameters(shader_params);
  Node->set_primitive_type(fw::sg::PrimitiveType::kTriangleList);
  Node->set_cast_shadows(false);
  rs.Scenegraph.add_node(Node);
}

bool ParticleRenderer::add_particle(RenderState &rs, int base_index, Particle *p, float offset_x, float offset_z) {
  fw::Camera *cam = fw::framework::get_instance()->get_camera();
  fw::Vector pos(p->pos[0] + offset_x, p->pos[1], p->pos[2] + offset_z);

  // only render if the (absolute, not wrapped) distance to the camera is < 50
  fw::Vector dir_to_cam = (cam->get_position() - pos);
  if (dir_to_cam.length_squared() > (50.0f * 50.0f))
    return false;

  // if we've already drawn the Particle this frame, don't do it again.
  if (p->draw_frame == _draw_frame)
    return false;
  p->draw_frame = _draw_frame;

  // the color_row is divided by this value to get the value between 0 and 1.
  float color_texture_factor = 1.0f / this->color_texture_->get_height();

  fw::Color color(p->alpha, (static_cast<float>(p->color1) + 0.5f) * color_texture_factor,
      (static_cast<float>(p->color2) + 0.5f) * color_texture_factor, p->color_factor);

  Matrix m = fw::scale(p->size);
  if (p->rotation != ParticleRotation::kDirection) {
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

  float aspect = p->rect.height / p->rect.width;

  fw::Vector v = cml::transform_point(m, fw::Vector(-0.5f, -0.5f * aspect, 0));
  rs.vertices.push_back(fw::vertex::xyz_c_uv(v[0], v[1], v[2], color.to_abgr(), p->rect.left, p->rect.top + p->rect.height));
  v = cml::transform_point(m, fw::Vector(-0.5f, 0.5f * aspect, 0));
  rs.vertices.push_back(fw::vertex::xyz_c_uv(v[0], v[1], v[2], color.to_abgr(), p->rect.left, p->rect.top));
  v = cml::transform_point(m, fw::Vector(0.5f, 0.5f * aspect, 0));
  rs.vertices.push_back(fw::vertex::xyz_c_uv(v[0], v[1], v[2], color.to_abgr(), p->rect.left + p->rect.width, p->rect.top));
  v = cml::transform_point(m, fw::Vector(0.5f, -0.5f * aspect, 0));
  rs.vertices.push_back(fw::vertex::xyz_c_uv(v[0], v[1], v[2], color.to_abgr(), p->rect.left + p->rect.width, p->rect.top + p->rect.height));

  rs.indices.push_back(base_index);
  rs.indices.push_back(base_index + 1);
  rs.indices.push_back(base_index + 2);
  rs.indices.push_back(base_index);
  rs.indices.push_back(base_index + 2);
  rs.indices.push_back(base_index + 3);

  return true;
}

void ParticleRenderer::render_particles(RenderState &rs, float offset_x, float offset_z) {
  for (ParticleRenderer::ParticleList::iterator it = rs.particles.begin(); it != rs.particles.end(); ++it) {
    Particle *p = *it;

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

void ParticleRenderer::render(sg::Scenegraph &Scenegraph, ParticleRenderer::ParticleList &particles) {
  if (particles.size() == 0)
    return;

  // make sure the Particle's pos is update
  for(Particle *p : particles) {
    p->pos = p->new_pos;
  }

  // sort the particles by texture, then by z-order
  sort_particles(particles);

  // create the render state that'll hold all our state variables
  RenderState rs(Scenegraph, particles);
  rs.Shader = shader_;
  rs.ShaderParameters = shader_params_;
  rs.particle_num = 0;
  rs.mode = ParticleEmitterConfig::kNormal;

  if (mgr_->get_wrap_x() > 1.0f && mgr_->get_wrap_z() > 1.0f) {
    for (int z = -1; z <= 1; z++) {
      for (int x = -1; x <= 1; x++) {
        float offset_x = x * mgr_->get_wrap_x();
        float offset_z = z * mgr_->get_wrap_z();
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
  for(const auto& vb : rs.vertex_buffers) {
    g_buffer_cache.release_vertex_buffer(vb);
  }
  for(const auto& ib : rs.index_buffers) {
    g_buffer_cache.release_index_buffer(ib);
  }

  // now this frame is over record it so we'll continue to draw particles next frame
  _draw_frame++;
}

void ParticleRenderer::sort_particles(ParticleRenderer::ParticleList &particles) {
  fw::Camera *cam = fw::framework::get_instance()->get_camera();
  fw::Vector const &cam_pos = cam->get_position();

  particles.sort(ParticleSorter(cam_pos));
}

}

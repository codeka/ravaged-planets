
#include <memory>

#include <boost/foreach.hpp>

#include <framework/scenegraph.h>
#include <framework/graphics.h>
#include <framework/framework.h>
#include <framework/camera.h>
#include <framework/logging.h>
#include <framework/exception.h>
#include <framework/misc.h>
#include <framework/shader.h>
#include <framework/shadows.h>
#include <framework/texture.h>
#include <framework/gui/gui.h>

static std::shared_ptr<fw::shader> shadow_shader;
static std::shared_ptr<fw::shader> basic_shader;

static bool is_rendering_shadow = false;
static std::shared_ptr<fw::shadow_source> shadowsrc;

static std::map<fw::sg::primitive_type, uint32_t> g_primitive_type_map;

static void ensure_primitive_type_map() {
  if (g_primitive_type_map.size() > 0)
    return;

  g_primitive_type_map[fw::sg::primitive_linestrip] = GL_LINE_STRIP;
  g_primitive_type_map[fw::sg::primitive_linelist] = GL_LINES;
  g_primitive_type_map[fw::sg::primitive_trianglelist] = GL_TRIANGLES;
  g_primitive_type_map[fw::sg::primitive_trianglestrip] = GL_TRIANGLE_STRIP;
}

namespace fw {

namespace sg {

//-------------------------------------------------------------------------------------
light::light() :
    _cast_shadows(false) {
}

light::light(fw::Vector const &pos, fw::Vector const &dir, bool cast_shadows) :
    _pos(pos), _dir(dir), _cast_shadows(cast_shadows) {
}

light::light(light const &copy) :
    _pos(copy._pos), _dir(copy._dir), _cast_shadows(copy._cast_shadows) {
}

light::~light() {
}

//-------------------------------------------------------------------------------------
node::node() :
    _world(fw::identity()), _parent(0), _cast_shadows(true), _primitive_type(primitive_whatever) {
}

node::~node() {
}

void node::add_child(std::shared_ptr<node> child) {
  child->_parent = this;
  _children.push_back(child);
}

void node::remove_child(std::shared_ptr<node> child) {
  auto it = std::find(_children.begin(), _children.end(), child);
  if (it != _children.end())
    _children.erase(it);
}

// Get the shader file to use. if we don't have one defined, look at our parent and keep looking up at our parents
// until we find one.
std::shared_ptr<fw::shader> node::get_shader() const {
  std::shared_ptr<fw::shader> shader = _shader;
  if (!shader) {
    node *parent = _parent;
    while (!shader && parent != 0) {
      shader = parent->_shader;
      parent = parent->_parent;
    }
  }

  return shader;
}

void node::render(scenegraph *sg, fw::Matrix const &model_matrix /*= fw::identity()*/) {
  // if we're not a shadow caster, and we're rendering shadows, don't render the node this time around
  if (is_rendering_shadow && !_cast_shadows) {
    return;
  }

  fw::Matrix transform(model_matrix * _world);
  if (_vb) {
    std::shared_ptr<fw::shader> shader = get_shader();
    if (is_rendering_shadow) {
      shader = shadow_shader;
    }

    fw::Camera *camera = sg->get_camera();
    if (camera == nullptr) {
      camera = fw::framework::get_instance()->get_camera();
    }

    if (!shader) {
      render_noshader(camera, transform);
    } else {
      render_shader(shader, camera, transform);
    }
  }

  // render the children as well (todo: pass transformations)
  BOOST_FOREACH(std::shared_ptr<node> &child_node, _children) {
    child_node->render(sg, transform);
  }
}

// this is called when we're rendering a given shader
void node::render_shader(std::shared_ptr<fw::shader> shader, fw::Camera *camera, fw::Matrix const &transform) {
  std::shared_ptr<fw::shader_parameters> parameters;
  if (_shader_params) {
    parameters = _shader_params;
  } else {
    parameters = shader->create_parameters();
  }

  // add the world_view and world_view_proj parameters as well as shadow parameters
  if (camera != nullptr) {
    fw::Matrix worldview = camera->get_view_matrix();
    worldview = transform * worldview;
    fw::Matrix worldviewproj = worldview * camera->get_projection_matrix();

    parameters->set_matrix("worldviewproj", worldviewproj);
    parameters->set_matrix("worldview", worldview);

    if (!is_rendering_shadow && shadowsrc) {
      fw::Matrix lightviewproj = transform * shadowsrc->get_camera().get_view_matrix();
      lightviewproj *= shadowsrc->get_camera().get_projection_matrix();

      fw::Matrix bias = fw::Matrix(
          0.5, 0.0, 0.0, 0.0,
          0.0, 0.5, 0.0, 0.0,
          0.0, 0.0, 0.5, 0.0,
          0.5, 0.5, 0.5, 1.0
      );

      parameters->set_matrix("lightviewproj", bias * lightviewproj);
      parameters->set_texture("shadow_map", shadowsrc->get_shadowmap()->get_depth_buffer());
    }
  }

  _vb->begin();
  shader->begin(parameters);
  if (_ib) {
    _ib->begin();
    FW_CHECKED(glDrawElements(g_primitive_type_map[_primitive_type], _ib->get_num_indices(), GL_UNSIGNED_SHORT, nullptr));
    _ib->end();
  } else {
    FW_CHECKED(glDrawArrays(g_primitive_type_map[_primitive_type], 0, _vb->get_num_vertices()));
  }
  shader->end();
  _vb->end();
}

void node::render_noshader(fw::Camera *camera, fw::Matrix const &transform) {
  if (!basic_shader) {
    basic_shader = fw::shader::create("basic.shader");
  }

  render_shader(basic_shader, camera, transform);
}

void node::populate_clone(std::shared_ptr<node> clone) {
  clone->_cast_shadows = _cast_shadows;
  clone->_primitive_type = _primitive_type;
  clone->_vb = _vb;
  clone->_ib = _ib;
  clone->_shader = _shader;
  if (_shader_params)
    clone->_shader_params = _shader_params->clone();
  clone->_parent = _parent;
  clone->_world = _world;

  // clone the children as well!
  BOOST_FOREACH(std::shared_ptr<node> child, _children) {
    clone->_children.push_back(child->clone());
  }
}

std::shared_ptr<node> node::clone() {
  std::shared_ptr<node> clone(new node());
  populate_clone(clone);
  return clone;
}

//-----------------------------------------------------------------------------------------

scenegraph::scenegraph()
    : _clear_color(fw::Color(1, 0, 0, 0)) {
}

scenegraph::~scenegraph() {
}

}

//-----------------------------------------------------------------------------------------
static const bool g_shadow_debug = false;

// renders the scene!
void render(sg::scenegraph &scenegraph, std::shared_ptr<fw::Framebuffer> render_target /*= nullptr*/,
    bool render_gui /*= true*/) {
  ensure_primitive_type_map();

  graphics *g = fw::framework::get_instance()->get_graphics();

  if (!shadow_shader) {
    shadow_shader = fw::shader::create("shadow.shader");
  }

  // set up the shadow sources that we'll need to render from first to get the various shadows going.
  std::vector<std::shared_ptr<shadow_source>> shadows;
  for(auto it = scenegraph.get_lights().begin(); it != scenegraph.get_lights().end(); ++it) {
    if ((*it)->get_cast_shadows()) {
      std::shared_ptr<shadow_source> shdwsrc(new shadow_source());
      shdwsrc->initialize(g_shadow_debug);

      light_camera &cam = shdwsrc->get_camera();
      cam.set_location((*it)->get_position());
      cam.set_direction((*it)->get_direction());

      shadows.push_back(shdwsrc);
    }
  }

  // render the shadowmap(s) first
  is_rendering_shadow = true;
  BOOST_FOREACH(shadowsrc, shadows) {
    shadowsrc->begin_scene();
    scenegraph.push_camera(&shadowsrc->get_camera());
    g->begin_scene();
    BOOST_FOREACH(std::shared_ptr<fw::sg::node> node, scenegraph.get_nodes()) {
      node->render(&scenegraph);
    }
    g->end_scene();
    scenegraph.pop_camera();
    shadowsrc->end_scene();
  }
  is_rendering_shadow = false;

  if (render_target) {
    g->set_render_target(render_target);
  }

  // now, render the main scene
  g->begin_scene(scenegraph.get_clear_color());
  BOOST_FOREACH(std::shared_ptr<fw::sg::node> node, scenegraph.get_nodes()) {
    node->render(&scenegraph);
  }

  // make sure the shadowsrc is empty
  std::shared_ptr<shadow_source> debug_shadowsrc;
  if (g_shadow_debug) {
    debug_shadowsrc = shadowsrc;
  }
  if (shadowsrc) {
    shadowsrc.reset();
  }

  if (render_gui) {
    // render the GUI now
    g->before_gui();

    if (g_shadow_debug && debug_shadowsrc) {
      std::shared_ptr<shader> shader = shader::create("gui.shader");
      std::shared_ptr<shader_parameters> shader_params = shader->create_parameters();
      // TODO: recalculating this every time seems wasteful
      fw::graphics *g = fw::framework::get_instance()->get_graphics();
      fw::Matrix pos_transform;
      cml::matrix_orthographic_RH(pos_transform, 0.0f,
          static_cast<float>(g->get_width()), static_cast<float>(g->get_height()), 0.0f, 1.0f, -1.0f, cml::z_clip_neg_one);
      pos_transform = fw::scale(fw::Vector(200.0f, 200.0f, 0.0f)) * fw::translation(fw::Vector(440.0f, 280.0f, 0)) * pos_transform;
      shader_params->set_matrix("pos_transform", pos_transform);
      shader_params->set_matrix("uv_transform", fw::identity());
      shader_params->set_texture("texsampler", debug_shadowsrc->get_shadowmap()->get_color_buffer());

      std::shared_ptr<vertex_buffer> vb = vertex_buffer::create<vertex::xyz_uv>();
      fw::vertex::xyz_uv vertices[4];
      vertices[0] = fw::vertex::xyz_uv(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      vertices[1] = fw::vertex::xyz_uv(0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
      vertices[2] = fw::vertex::xyz_uv(1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
      vertices[3] = fw::vertex::xyz_uv(1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
      vb->set_data(4, vertices);

      std::shared_ptr<index_buffer> ib = std::shared_ptr<fw::index_buffer>(new fw::index_buffer());
      uint16_t indices[4];
      indices[0] = 0;
      indices[1] = 1;
      indices[2] = 2;
      indices[3] = 3;
      ib->set_data(4, indices);

      vb->begin();
      ib->begin();
      shader->begin(shader_params);
      FW_CHECKED(glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr));
      shader->end();
      ib->end();
      vb->end();
    }

    framework::get_instance()->get_gui()->render();
    g->after_gui();
  }

  g->end_scene();
  if (render_target) {
    g->set_render_target(nullptr);
  } else {
    g->present();
  }
}
}

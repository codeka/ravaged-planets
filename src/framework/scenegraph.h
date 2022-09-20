#pragma once

#include <memory>
#include <stack>

#include <framework/camera.h>
#include <framework/color.h>
#include <framework/graphics.h>
#include <framework/shader.h>
#include <framework/shadows.h>
#include <framework/texture.h>
#include <framework/vector.h>

namespace fw::sg {

class scenegraph;

enum primitive_type {
  primitive_linestrip,
  primitive_linelist,
  primitive_trianglestrip,
  primitive_trianglelist,
  primitive_whatever
};

// represents the properties of a light that we'll need to add to the
// scene (we'll need at least one light-source of course!
class light {
private:
  fw::Vector _pos;
  fw::Vector _dir;
  bool _cast_shadows;

public:
  light();
  light(fw::Vector const &pos, fw::Vector const &dir, bool cast_shadows);
  light(light const &copy);
  ~light();

  void set_position(fw::Vector const &pos) {
    _pos = pos;
  }
  fw::Vector get_position() const {
    return _pos;
  }

  void set_direction(fw::Vector const &dir) {
    _dir = dir;
  }
  fw::Vector get_direction() const {
    return _dir;
  }

  void set_cast_shadows(bool cast_shadows) {
    _cast_shadows = cast_shadows;
  }
  bool get_cast_shadows() const {
    return _cast_shadows;
  }
};

// this is the base "scene graph node"
class node {
private:
  bool _cast_shadows;

  primitive_type _primitive_type;
  std::shared_ptr<fw::VertexBuffer> vb_;
  std::shared_ptr<fw::IndexBuffer> ib_;
  std::shared_ptr<fw::shader> shader_;
  std::shared_ptr<fw::shader_parameters> _shader_params;

  // Renders the node if the shader file is null (basically just uses the basic shader).
  void render_noshader(fw::Camera *camera, fw::Matrix const &transform);

protected:
  node *_parent;
  std::vector<std::shared_ptr<node> > _children;
  fw::Matrix _world;

  // this is called when we're rendering a given shader
  virtual void render_shader(std::shared_ptr<fw::shader> shader, fw::Camera *camera, fw::Matrix const &transform);

  // called by clone() to populate the clone
  virtual void populate_clone(std::shared_ptr<node> clone);
public:
  node();
  virtual ~node();

  void add_child(std::shared_ptr<node> child);
  void remove_child(std::shared_ptr<node> child);
  int get_num_children() const {
    return _children.size();
  }
  std::shared_ptr<node> get_child(int index) const {
    return _children[index];
  }

  void set_world_matrix(fw::Matrix const &m) {
    _world = m;
  }
  fw::Matrix &get_world_matrix() {
    return _world;
  }

  void set_vertex_buffer(std::shared_ptr<fw::VertexBuffer> vb) {
    vb_ = vb;
  }
  std::shared_ptr<fw::VertexBuffer> get_vertex_buffer() const {
    return vb_;
  }

  void set_index_buffer(std::shared_ptr<fw::IndexBuffer> ib) {
    ib_ = ib;
  }
  std::shared_ptr<fw::IndexBuffer> get_index_buffer() const {
    return ib_;
  }

  void set_shader(std::shared_ptr<fw::shader> shader) {
    shader_ = shader;
  }
  std::shared_ptr<fw::shader> get_shader() const;

  void set_shader_parameters(std::shared_ptr<fw::shader_parameters> shader_params) {
    _shader_params = shader_params;
  }
  std::shared_ptr<fw::shader_parameters> get_shader_parameters() const {
    return _shader_params;
  }

  void set_cast_shadows(bool cast_shadows) {
    _cast_shadows = cast_shadows;
  }
  bool get_cast_shadows() const {
    return _cast_shadows;
  }

  void set_primitive_type(primitive_type pt) {
    _primitive_type = pt;
  }
  primitive_type get_primitive_type() const {
    return _primitive_type;
  }

  // this is called by the scenegraph itself when it's time to render
  virtual void render(scenegraph *sg, fw::Matrix const &model_matrix = fw::identity());

  // Creates a clone of this node (it's a "shallow" clone in that the vertex_buffer, index_buffer and shader will be
  // shared but matrix and shader_parameters will be new)
  virtual std::shared_ptr<node> clone();
};

// this class manages the scene graph.
class scenegraph {
public:
  typedef std::vector<std::shared_ptr<light> > light_coll;
  typedef std::vector<std::shared_ptr<node> > node_coll;

private:
  light_coll _lights;
  node_coll _root_nodes;
  fw::Color _clear_color;
  std::stack<fw::Camera *> _camera_stack;

public:
  scenegraph();
  ~scenegraph();

  // add a light to the scene. the pointer must be valid for the
  void add_light(std::shared_ptr<light> &l) {
    _lights.push_back(l);
  }
  light_coll const &get_lights() const {
    return _lights;
  }

  void add_node(std::shared_ptr<node> node) {
    _root_nodes.push_back(node);
  }
  node_coll const &get_nodes() const {
    return _root_nodes;
  }

  void set_clear_color(fw::Color color) {
    _clear_color = color;
  }
  fw::Color get_clear_color() const {
    return _clear_color;
  }

  void push_camera(fw::Camera *cam) {
    _camera_stack.push(cam);
  }
  void pop_camera() {
    _camera_stack.pop();
  }
  fw::Camera *get_camera() const {
    if (_camera_stack.empty()) {
      return nullptr;
    }
    return _camera_stack.top();
  }
};
}

namespace fw {
void render(fw::sg::scenegraph &sg, std::shared_ptr<fw::Framebuffer> render_target = nullptr, bool render_gui = true);
}

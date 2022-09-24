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

class Scenegraph;

enum PrimitiveType {
  kUnknownPrimitiveType,
  kLineStrip,
  kLineList,
  kTriangleStrip,
  kTriangleList
};

// represents the properties of a Light that we'll need to add to the
// scene (we'll need at least one Light-source of course!
class Light {
private:
  fw::Vector pos_;
  fw::Vector dir_;
  bool cast_shadows_;

public:
  Light();
  Light(fw::Vector const &pos, fw::Vector const &dir, bool cast_shadows);
  Light(Light const &copy);
  ~Light();

  void set_position(fw::Vector const &pos) {
    pos_ = pos;
  }
  fw::Vector get_position() const {
    return pos_;
  }

  void set_direction(fw::Vector const &dir) {
    dir_ = dir;
  }
  fw::Vector get_direction() const {
    return dir_;
  }

  void set_cast_shadows(bool cast_shadows) {
    cast_shadows_ = cast_shadows;
  }
  bool get_cast_shadows() const {
    return cast_shadows_;
  }
};

// this is the base "scene graph Node"
class Node {
private:
  bool cast_shadows_;

  PrimitiveType _primitive_type;
  std::shared_ptr<fw::VertexBuffer> vb_;
  std::shared_ptr<fw::IndexBuffer> ib_;
  std::shared_ptr<fw::Shader> shader_;
  std::shared_ptr<fw::ShaderParameters> shader_params_;

  // Renders the Node if the Shader file is null (basically just uses the basic Shader).
  void render_noshader(fw::Camera *camera, fw::Matrix const &transform);

protected:
  Node *_parent;
  std::vector<std::shared_ptr<Node> > _children;
  fw::Matrix world_;

  // this is called when we're rendering a given Shader
  virtual void render_shader(std::shared_ptr<fw::Shader> Shader, fw::Camera *camera, fw::Matrix const &transform);

  // called by clone() to populate the clone
  virtual void populate_clone(std::shared_ptr<Node> clone);
public:
  Node();
  virtual ~Node();

  void add_child(std::shared_ptr<Node> child);
  void remove_child(std::shared_ptr<Node> child);
  int get_num_children() const {
    return _children.size();
  }
  std::shared_ptr<Node> get_child(int index) const {
    return _children[index];
  }

  void set_world_matrix(fw::Matrix const &m) {
    world_ = m;
  }
  fw::Matrix &get_world_matrix() {
    return world_;
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

  void set_shader(std::shared_ptr<fw::Shader> Shader) {
    shader_ = Shader;
  }
  std::shared_ptr<fw::Shader> get_shader() const;

  void set_shader_parameters(std::shared_ptr<fw::ShaderParameters> shader_params) {
    shader_params_ = shader_params;
  }
  std::shared_ptr<fw::ShaderParameters> get_shader_parameters() const {
    return shader_params_;
  }

  void set_cast_shadows(bool cast_shadows) {
    cast_shadows_ = cast_shadows;
  }
  bool get_cast_shadows() const {
    return cast_shadows_;
  }

  void set_primitive_type(PrimitiveType pt) {
    _primitive_type = pt;
  }
  PrimitiveType get_primitive_type() const {
    return _primitive_type;
  }

  // this is called by the Scenegraph itself when it's time to render
  virtual void render(Scenegraph *sg, fw::Matrix const &model_matrix = fw::identity());

  // Creates a clone of this Node (it's a "shallow" clone in that the vertex_buffer, index_buffer and Shader will be
  // shared but matrix and ShaderParameters will be new)
  virtual std::shared_ptr<Node> clone();
};

// this class manages the scene graph.
class Scenegraph {
public:
  typedef std::vector<std::shared_ptr<Light> > light_coll;
  typedef std::vector<std::shared_ptr<Node> > node_coll;

private:
  light_coll lights_;
  node_coll root_nodes_;
  fw::Color clear_color_;
  std::stack<fw::Camera *> camera_stack_;

public:
  Scenegraph();
  ~Scenegraph();

  // add a Light to the scene. the pointer must be valid for the
  void add_light(std::shared_ptr<Light> &l) {
    lights_.push_back(l);
  }
  light_coll const &get_lights() const {
    return lights_;
  }

  void add_node(std::shared_ptr<Node> Node) {
    root_nodes_.push_back(Node);
  }
  node_coll const &get_nodes() const {
    return root_nodes_;
  }

  void set_clear_color(fw::Color color) {
    clear_color_ = color;
  }
  fw::Color get_clear_color() const {
    return clear_color_;
  }

  void push_camera(fw::Camera *cam) {
    camera_stack_.push(cam);
  }
  void pop_camera() {
    camera_stack_.pop();
  }
  fw::Camera *get_camera() const {
    if (camera_stack_.empty()) {
      return nullptr;
    }
    return camera_stack_.top();
  }
};
}

namespace fw {
void render(fw::sg::Scenegraph &sg, std::shared_ptr<fw::Framebuffer> render_target = nullptr, bool render_gui = true);
}

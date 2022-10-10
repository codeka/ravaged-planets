#pragma once

#include <list>
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

// This is the base scene graph Node contains all the information needed to render a single object in the scene. In
// general, this will translate to a single OpenGL draw call.
class Node {
private:
  bool cast_shadows_;

  PrimitiveType primitive_type_;
  std::shared_ptr<fw::VertexBuffer> vb_;
  std::shared_ptr<fw::IndexBuffer> ib_;
  std::shared_ptr<fw::Shader> shader_;
  std::shared_ptr<fw::ShaderParameters> shader_params_;

  // Renders the Node if the Shader file is null (basically just uses the basic Shader).
  void render_noshader(fw::Camera *camera, fw::Matrix const &transform);

protected:
  Node *parent_;
  std::vector<std::shared_ptr<Node> > children_;
  fw::Matrix world_;

  // this is called when we're rendering a given Shader
  virtual void render_shader(std::shared_ptr<fw::Shader> shader, fw::Camera *camera, fw::Matrix const &transform);

  // called by clone() to populate the clone
  virtual void populate_clone(std::shared_ptr<Node> clone);
public:
  Node();
  virtual ~Node();

  void add_child(std::shared_ptr<Node> child);
  void remove_child(std::shared_ptr<Node> child);
  void clear_children();
  int get_num_children() const {
    return children_.size();
  }
  std::shared_ptr<Node> get_child(int index) const {
    return children_[index];
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
    primitive_type_ = pt;
  }
  PrimitiveType get_primitive_type() const {
    return primitive_type_;
  }

  // this is called by the Scenegraph itself when it's time to render
  virtual void render(Scenegraph *sg, fw::Matrix const &model_matrix = fw::identity());

  // Creates a clone of this Node (it's a "shallow" clone in that the vertex_buffer, index_buffer and Shader will be
  // shared but matrix and ShaderParameters will be new)
  virtual std::shared_ptr<Node> clone();
};

// The scenegraph is the main interface between the "update" thread and the "render" thread. There should be no data
// shared between these two threads that is not part of the scenegraph. The scenegraph is a coherent data structure
// (that is, it persists from frame-to-frame). You can add nodes to it, and then post closures to the scenegraph to
// update the nodes on the render thread.
class Scenegraph {
private:
  std::vector<std::shared_ptr<Light>> lights_;
  std::vector<std::shared_ptr<Node>> root_nodes_;
  fw::Color clear_color_;
  std::stack<fw::Camera *> camera_stack_;

public:
  Scenegraph();
  ~Scenegraph();

  // add a Light to the scene. the pointer must be valid for the
  void add_light(std::shared_ptr<Light> &l) {
    lights_.push_back(l);
  }
  std::vector<std::shared_ptr<Light>> const &get_lights() const {
    return lights_;
  }

  void add_node(std::shared_ptr<Node> Node) {
    root_nodes_.push_back(Node);
  }
  std::vector<std::shared_ptr<Node>> const &get_nodes() const {
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

// The ScenegraphManager manages access to the scenegraph. Because manipulation of the scenegraph can only occur on the
// render thread, this class is mostly just an interface for queuing closures to run on the render thread.
class ScenegraphManager {
private:
  std::mutex mutex_;
  Scenegraph scenegraph_;

  // We have two lists of closures. To keep things efficient, one list is being used by the render thread, the other
  // is being used by the update thread. We lock the mutex only long enough to switch which is which. update_index_ is
  // the index of the list we are accessing from the update thread.
  std::list<std::function<void(Scenegraph&)>> closures_[2];
  int update_index_ = 0;
public:

  // Called on the update thread. Enqueues the given closure to run on the render thread. We'll pass it the scenegraph
  // that you can update, or whatever is needed.
  void enqueue(std::function<void(Scenegraph&)> closure);

  // Called on the render thread, before rendering a frame. We'll run all of the enqueued closures.
  void before_render();

  // Called on the render thread, returns the scenegraph.
  Scenegraph& get_scenegraph();
};

}  // namespace fw::sg

namespace fw {
void render(fw::sg::Scenegraph &sg, std::shared_ptr<fw::Framebuffer> render_target = nullptr, bool render_gui = true);
}

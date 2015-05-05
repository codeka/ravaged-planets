#pragma once

#include <memory>

#include "vector.h"
#include "colour.h"

namespace fw {

class shadow_source;
class vertex_buffer;
class index_buffer;
class effect;
class effect_parameters;
class texture;

namespace sg {

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
  fw::vector _pos;
  fw::vector _dir;
  bool _cast_shadows;

public:
  light();
  light(fw::vector const &pos, fw::vector const &dir, bool cast_shadows);
  light(light const &copy);
  ~light();

  void set_position(fw::vector const &pos) {
    _pos = pos;
  }
  fw::vector get_position() const {
    return _pos;
  }

  void set_direction(fw::vector const &dir) {
    _dir = dir;
  }
  fw::vector get_direction() const {
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
  std::shared_ptr<fw::vertex_buffer> _vb;
  std::shared_ptr<fw::index_buffer> _ib;
  std::shared_ptr<fw::effect> _fx;
  std::shared_ptr<fw::effect_parameters> _fx_params;

  // Renders the node if the effect file is null (basically just uses the basic effect).
  void render_nofx();

protected:
  node *_parent;
  std::vector<std::shared_ptr<node> > _children;
  fw::matrix _world;

  // this is called when we're rendering a given effect
  virtual void render_fx(std::shared_ptr<fw::effect> fx);

  // called to set any additional parameters on the given effect
  virtual void setup_effect(std::shared_ptr<fw::effect> fx);

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

  void set_world_matrix(fw::matrix const &m) {
    _world = m;
  }
  fw::matrix &get_world_matrix() {
    return _world;
  }

  void set_vertex_buffer(std::shared_ptr<fw::vertex_buffer> vb) {
    _vb = vb;
  }
  std::shared_ptr<fw::vertex_buffer> get_vertex_buffer() const {
    return _vb;
  }

  void set_index_buffer(std::shared_ptr<fw::index_buffer> ib) {
    _ib = ib;
  }
  std::shared_ptr<fw::index_buffer> get_index_buffer() const {
    return _ib;
  }

  void set_effect(std::shared_ptr<fw::effect> fx) {
    _fx = fx;
  }
  std::shared_ptr<fw::effect> get_effect() const;

  void set_effect_parameters(
      std::shared_ptr<fw::effect_parameters> fx_params) {
    _fx_params = fx_params;
  }
  std::shared_ptr<fw::effect_parameters> get_effect_parameters() const {
    return _fx_params;
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
  virtual void render(scenegraph *sg);

  // creates a clone of this node (it's a "shallow" clone in that the vertex_buffer,
  // index_buffer and effect will be shared but matrix and effect_parameters will be
  // new)
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

public:
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
};
}

void render(fw::sg::scenegraph &sg, fw::texture *render_target = 0,
    fw::colour clear_colour = fw::colour(1, 0, 0, 0));
}

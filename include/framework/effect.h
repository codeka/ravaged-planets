#pragma once

#include <string>
#include <map>
#include <memory>
#include <boost/noncopyable.hpp>

#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/vector.h>
#include <framework/scenegraph.h>

struct effect_data;

namespace fw {

class graphics;
class texture;
class effect;
class colour;
class vertex_buffer;
class index_buffer;

// you can pass this to an effect to set a bunch of parameters all at once
class effect_parameters: private boost::noncopyable {
private:
  friend class effect;

  std::map<std::string, std::shared_ptr<texture> > _textures;
  std::map<std::string, std::shared_ptr<vertex_buffer>> _vertex_buffers;
  std::map<std::string, matrix> _matrices;
  std::map<std::string, vector> _vectors;
  std::map<std::string, colour> _colours;
  std::map<std::string, float> _scalars;

  effect_parameters();
  void apply(effect *e) const;

public:
  ~effect_parameters();

  void set_texture(std::string const &name, std::shared_ptr<texture> const &t);
  void set_vertex_buffer(std::string const &name, std::shared_ptr<vertex_buffer> const &vb);
  void set_index_buffer(std::string const &name, std::shared_ptr<index_buffer> const &ib);
  void set_matrix(std::string const &name, matrix const &m);
  void set_vector(std::string const &name, vector const &v);
  void set_colour(std::string const &name, colour const &c);
  void set_scalar(std::string const &name, float f);

  std::shared_ptr<effect_parameters> clone();
};

// this class wraps CgFX files and allows us to automatically reload them, and so on.
class effect {
private:
  friend class effect_parameters;

  std::shared_ptr<effect_data> _data;

public:
  effect();
  ~effect();

  void initialise(boost::filesystem::path const &name);

  // creates an effect_parameters that you'll pass to begin() in order to set up the parameters for this rendering.
  std::shared_ptr<effect_parameters> create_parameters();

  void set_texture(char const *name, texture const &t);
  void set_matrix(char const *name, matrix const &m);
  void set_vector(char const *name, vector const &v);
  void set_colour(char const *name, colour const &c);
  void set_scalar(char const *name, float f);

  void render(std::shared_ptr<effect_parameters> parameters, int num_primitives, fw::sg::primitive_type primitive_type,
      index_buffer *idx_buffer);
//  void begin(std::shared_ptr<effect_parameters> parameters);
//  void end();
};

}

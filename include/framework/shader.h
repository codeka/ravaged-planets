#pragma once

#include <string>
#include <map>
#include <memory>
#include <boost/noncopyable.hpp>

#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/vector.h>

namespace fw {

class graphics;
class texture;
class shader;
class colour;
class vertex_buffer;
class index_buffer;

// you can pass this to a shader to set a bunch of parameters all at once
class shader_parameters: private boost::noncopyable {
private:
  friend class shader;

  std::map<std::string, std::shared_ptr<texture> > _textures;
  std::map<std::string, matrix> _matrices;
  std::map<std::string, vector> _vectors;
  std::map<std::string, colour> _colours;
  std::map<std::string, float> _scalars;

  shader_parameters();
  void apply(shader *e) const;

public:
  ~shader_parameters();

  void set_texture(std::string const &name, std::shared_ptr<texture> const &t);
  void set_matrix(std::string const &name, matrix const &m);
  void set_vector(std::string const &name, vector const &v);
  void set_colour(std::string const &name, colour const &c);
  void set_scalar(std::string const &name, float f);

  std::shared_ptr<shader_parameters> clone();
};

/** Contains information about a shader variable in a complied shader program. */
class shader_variable {
public:
  GLint location;
  std::string name;
  GLint size;
  GLenum type;

  shader_variable();
  shader_variable(GLint location, std::string name, GLint size, GLenum type);
};

// this shader wraps shader files and allows us to automatically reload them, and so on.
class shader {
private:
  shader();

  friend class shader_parameters;

  boost::filesystem::path _filename;
  GLuint _program_id;
  std::map<std::string, shader_variable> _shader_variables;

  void load(fw::graphics *g, boost::filesystem::path const &full_path);

public:
  ~shader();

  static std::shared_ptr<shader> create(std::string const &name);

  // creates an shader_parameters that you'll pass to begin() in order to set up the parameters for this rendering.
  std::shared_ptr<shader_parameters> create_parameters();

  void begin(std::shared_ptr<shader_parameters> parameters);
  void end();
};

}

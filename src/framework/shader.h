#pragma once

#include <string>
#include <map>
#include <memory>
#include <boost/noncopyable.hpp>

#include <framework/color.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/texture.h>
#include <framework/vector.h>

namespace fw {
class shader;
class shader_program;

// you can pass this to a shader to set a bunch of parameters all at once
class shader_parameters: private boost::noncopyable {
private:
  friend class shader;

  std::string _program_name;
  std::map<std::string, std::shared_ptr<fw::Texture> > _textures;
  std::map<std::string, Matrix> _matrices;
  std::map<std::string, Vector> _vectors;
  std::map<std::string, Color> _colors;
  std::map<std::string, float> _scalars;

  shader_parameters();
  void apply(shader_program *prog) const;

public:
  ~shader_parameters();

  void set_program_name(std::string const &name);
  void set_texture(std::string const &name, std::shared_ptr<fw::Texture> const &t);
  void set_matrix(std::string const &name, Matrix const &m);
  void set_vector(std::string const &name, Vector const &v);
  void set_color(std::string const &name, Color const &c);
  void set_scalar(std::string const &name, float f);

  std::shared_ptr<shader_parameters> clone();
};

/** Contains information about a shader variable in a complied shader program. */
class shader_variable {
public:
  bool valid;
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
  boost::filesystem::path _filename;
  std::map<std::string, shader_program *> _programs;
  std::string _default_program_name;

  shader();
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

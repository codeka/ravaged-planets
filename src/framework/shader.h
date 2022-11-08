#pragma once

#include <string>
#include <map>
#include <memory>
#include <boost/noncopyable.hpp>

#include <framework/color.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/math.h>
#include <framework/texture.h>

namespace fw {
class Shader;
class ShaderProgram;

// you can pass this to a Shader to set a bunch of parameters all at once
class ShaderParameters {
private:
  friend class Shader;

  std::string program_name_;
  std::map<std::string, std::shared_ptr<fw::TextureBase>> textures_;
  std::map<std::string, Matrix> matrices_;
  std::map<std::string, Vector> vectors_;
  std::map<std::string, Color> colors_;
  std::map<std::string, float> scalars_;

  ShaderParameters();
  void apply(ShaderProgram *prog) const;

public:
  ~ShaderParameters();

  void set_program_name(std::string const &name);
  void set_texture(std::string const &name, std::shared_ptr<fw::Texture> const &t);
  void set_texture(std::string const& name, std::shared_ptr<fw::TextureArray> const& t);
  void set_matrix(std::string const &name, Matrix const &m);
  void set_vector(std::string const &name, Vector const &v);
  void set_color(std::string const &name, Color const &c);
  void set_scalar(std::string const &name, float f);

  std::shared_ptr<ShaderParameters> clone();
};

/** Contains information about a Shader variable in a complied Shader program. */
class ShaderVariable {
public:
  bool valid;
  GLint location;
  std::string name;
  GLint size;
  GLenum type;

  ShaderVariable();
  ShaderVariable(GLint location, std::string name, GLint size, GLenum type);
};

// this Shader wraps Shader files and allows us to automatically reload them, and so on.
class Shader {
private:
  boost::filesystem::path filename_;
  std::map<std::string, ShaderProgram *> programs_;
  std::string default_program_name_;

  Shader();
  void load(fw::Graphics *g, boost::filesystem::path const &full_path);

public:
  ~Shader();

  static std::shared_ptr<Shader> create(std::string const &name);

  // creates an ShaderParameters that you'll pass to begin() in order to set up the parameters for this rendering.
  std::shared_ptr<ShaderParameters> create_parameters();

  void begin(std::shared_ptr<ShaderParameters> parameters);
  void end();
};

}

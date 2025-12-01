#pragma once

#include <filesystem>
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
public:
  ShaderParameters();
  ~ShaderParameters();

  void set_program_name(std::string_view name);
  void set_texture(std::string_view name, std::shared_ptr<fw::Texture> const &t);
  void set_texture(std::string_view name, std::shared_ptr<fw::TextureArray> const& t);
  void set_matrix(std::string_view name, Matrix const &m);
  void set_vector(std::string_view name, Vector const &v);
  void set_color(std::string_view name, Color const &c);
  void set_scalar(std::string_view name, float f);

  std::shared_ptr<ShaderParameters> Clone();
private:
  friend class Shader;

  std::string program_name_;
  std::map<std::string, std::shared_ptr<fw::TextureBase>> textures_;
  std::map<std::string, Matrix> matrices_;
  std::map<std::string, Vector> vectors_;
  std::map<std::string, Color> colors_;
  std::map<std::string, float> scalars_;

  void Apply(ShaderProgram &prog) const;

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
public:
  Shader();
  ~Shader();

  static fw::StatusOr<std::shared_ptr<Shader>> Create(std::string_view name);

  // Creates a shader with the given name, or returns an empty shader if we cannot.
  static std::shared_ptr<Shader> CreateOrEmpty(std::string_view name);

  // creates an ShaderParameters that you'll pass to begin() in order to set up the parameters
  // for this rendering.
  std::shared_ptr<ShaderParameters> CreateParameters();

  void Begin(std::shared_ptr<ShaderParameters> parameters);
  void End();
private:
  std::filesystem::path filename_;
  std::map<std::string, std::shared_ptr<ShaderProgram>> programs_;
  std::string default_program_name_;

  fw::Status Load(fw::Graphics *g, std::filesystem::path const &full_path);
};

}

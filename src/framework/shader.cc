#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <framework/misc.h>
#include <framework/framework.h>
#include <framework/colour.h>
#include <framework/graphics.h>
#include <framework/texture.h>
#include <framework/settings.h>
#include <framework/logging.h>
#include <framework/paths.h>
#include <framework/vector.h>
#include <framework/exception.h>
#include <framework/scenegraph.h>
#include <framework/vertex_buffer.h>
#include <framework/index_buffer.h>
#include <framework/shader.h>

namespace fs = boost::filesystem;

//-------------------------------------------------------------------------
// This is a cache of .shader files, so we don't have to load them over and over...
class shader_cache {
private:
  typedef std::map<fs::path, std::shared_ptr<fw::shader> > shader_map;
  shader_map _shaders;

public:
  std::shared_ptr<fw::shader> get_shader(fs::path const &name);
  void add_shader(fs::path const &name, std::shared_ptr<fw::shader> data);

  void clear_cache();
};

std::shared_ptr<fw::shader> shader_cache::get_shader(fs::path const &name) {
  shader_map::iterator it = _shaders.find(name);
  if (it == _shaders.end())
    return std::shared_ptr<fw::shader>();

  return it->second;
}

void shader_cache::add_shader(fs::path const &name, std::shared_ptr<fw::shader> data) {
  _shaders[name] = data;
}

void shader_cache::clear_cache() {
  _shaders.clear();
}

//-----------------------------------------------------------------------------

namespace {
shader_cache g_cache;
void compile_shader(GLuint shader_id, std::string filename);
void link_shader(GLuint program_id, GLuint vertex_shader_id, GLuint fragment_shader_id);
}

namespace fw {
//-------------------------------------------------------------------------
shader_parameters::shader_parameters() {
}

shader_parameters::~shader_parameters() {
}

void shader_parameters::set_texture(std::string const &name, std::shared_ptr<texture> const &tex) {
  _textures[name] = tex;
}

void shader_parameters::set_matrix(std::string const &name, matrix const &m) {
  _matrices[name] = m;
}

void shader_parameters::set_vector(std::string const &name, vector const &v) {
  _vectors[name] = v;
}

void shader_parameters::set_colour(std::string const &name, colour const &c) {
  _colours[name] = c;
}

void shader_parameters::set_scalar(std::string const &name, float f) {
  _scalars[name] = f;
}

std::shared_ptr<shader_parameters> shader_parameters::clone() {
  std::shared_ptr<shader_parameters> clone(new shader_parameters());
  clone->_textures = _textures;
  clone->_matrices = _matrices;
  clone->_vectors = _vectors;
  clone->_colours = _colours;
  clone->_scalars = _scalars;
  return clone;
}

void shader_parameters::apply(shader *e) const {
  int texture_unit = 0;
  for (auto it = _textures.begin(); it != _textures.end(); ++it) {
    shader_variable const &var = e->_shader_variables[it->first];
    if (var.valid) {
      glActiveTexture(GL_TEXTURE0 + texture_unit);
      std::shared_ptr<fw::texture> texture = it->second;
      texture->bind();
      FW_CHECKED(glUniform1i(var.location, 0));
      texture_unit ++;
    }
  }

  for (std::map<std::string, matrix>::const_iterator it = _matrices.begin(); it != _matrices.end(); ++it) {
    shader_variable const &var = e->_shader_variables[it->first];
    if (var.valid) {
      FW_CHECKED(glUniformMatrix4fv(var.location, 1, GL_FALSE, it->second.data()));
    }
  }

  for (std::map<std::string, vector>::const_iterator it = _vectors.begin(); it != _vectors.end(); ++it) {
    shader_variable const &var = e->_shader_variables[it->first];
    if (var.valid) {
      FW_CHECKED(glUniform3fv(var.location, 1, it->second.data()));
    }
  }

  for (std::map<std::string, colour>::const_iterator it = _colours.begin(); it != _colours.end(); ++it) {
    shader_variable const &var = e->_shader_variables[it->first];
    if (var.valid) {
      FW_CHECKED(glUniform4f(var.location, it->second.r, it->second.g, it->second.b, it->second.a));
    }
  }

  for (std::map<std::string, float>::const_iterator it = _scalars.begin(); it != _scalars.end(); ++it) {
    shader_variable const &var = e->_shader_variables[it->first];
    if (var.valid) {
      FW_CHECKED(glUniform1f(var.location, it->second));
    }
  }
}

//-------------------------------------------------------------------------

shader_variable::shader_variable() :
  location(-1), name(""), size(0), type(0), valid(false) {
}

shader_variable::shader_variable(GLint location, std::string name, GLint size, GLenum type) :
  location(location), name(name), size(size), type(type), valid(true) {
}

//-------------------------------------------------------------------------
shader::shader() : _program_id(0) {
}

shader::~shader() {
}

std::shared_ptr<shader> shader::create(std::string const &filename) {
  fw::graphics *g = fw::framework::get_instance()->get_graphics();

  std::shared_ptr<shader> shdr = g_cache.get_shader(filename);
  if (!shdr) {
    shdr = std::shared_ptr<shader>(new shader());
    shdr->load(g, fw::resolve("share/ravaged-planets/shaders/" + filename));
    g_cache.add_shader(filename, shdr);
  }

  return shdr;
}

void shader::begin(std::shared_ptr<shader_parameters> parameters) {
  FW_CHECKED(glUseProgram(_program_id));
  if (parameters) {
    parameters->apply(this);
  }

#ifdef DEBUG
  GLint status;
  FW_CHECKED(glValidateProgram(_program_id));
  glGetProgramiv(_program_id, GL_VALIDATE_STATUS, &status);
  if (status != GL_TRUE) {
    GLint log_length;
    glGetProgramiv(_program_id, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> error_message(log_length);
    glGetProgramInfoLog(_program_id, log_length, nullptr, &error_message[0]);

    fw::debug << "glValidateProgram error: " << &error_message[0] << std::endl;
  }
#endif
}

void shader::end() {
  FW_CHECKED(glUseProgram(0));
}

std::shared_ptr<shader_parameters> shader::create_parameters() {
  std::shared_ptr<shader_parameters> params(new shader_parameters());
  return params;
}

void shader::load(fw::graphics *g, fs::path const &full_path) {
  std::shared_ptr<shader> data(new shader());
  _filename = full_path;
  GLint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
  GLint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
  _program_id = glCreateProgram();

  compile_shader(vertex_shader_id, full_path.string() + ".vert");
  compile_shader(fragment_shader_id, full_path.string() + ".frag");
  link_shader(_program_id, vertex_shader_id, fragment_shader_id);

  int num_uniforms;
  FW_CHECKED(glGetProgramiv(_program_id, GL_ACTIVE_UNIFORMS, &num_uniforms));
  GLchar buffer[1024];
  for (int i = 0; i < num_uniforms; i++) {
    GLint size;
    GLint length;
    GLenum type;
    FW_CHECKED(glGetActiveUniform(_program_id, i, sizeof(buffer), &size, &length, &type, buffer));
    GLint location = glGetUniformLocation(_program_id, buffer);
    std::string name(buffer);
    _shader_variables[name] = shader_variable(location, name, size, type);
  }

  fw::debug << "loaded shader from: " << full_path.string() << std::endl;
}

}

//-----------------------------------------------------------------------------
namespace {

std::string read_shader(std::string filename) {
  std::stringstream lines;
  std::ifstream stream(filename, std::ios::in);
  if (stream.is_open()) {
    std::string line;
    while (std::getline(stream, line)) {
      lines << line << std::endl;
    }
    stream.close();
  } else {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(filename));
  }
  return lines.str();
}

void compile_shader(GLuint shader_id, std::string filename) {
  std::string lines = read_shader(filename);
  char const *shader_source = lines.c_str();
  FW_CHECKED(glShaderSource(shader_id, 1, &shader_source, nullptr));
  FW_CHECKED(glCompileShader(shader_id));

  GLint status;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    GLint log_length;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> error_message(log_length);
    glGetShaderInfoLog(shader_id, log_length, nullptr, &error_message[0]);

    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(std::string(&error_message[0])));
  }
}

void link_shader(GLuint program_id, GLuint vertex_shader_id, GLuint fragment_shader_id) {
  FW_CHECKED(glAttachShader(program_id, vertex_shader_id));
  FW_CHECKED(glAttachShader(program_id, fragment_shader_id));
  FW_CHECKED(glLinkProgram(program_id));

  GLint status;
  glGetProgramiv(program_id, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    GLint log_length;
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> error_message(log_length);
    glGetProgramInfoLog(program_id, log_length, nullptr, &error_message[0]);

    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(std::string(&error_message[0])));
  }
}


}

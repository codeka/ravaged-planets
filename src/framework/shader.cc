#include <string>
#include <sstream>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
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
#include <framework/shader.h>
#include <framework/xml.h>

namespace fs = boost::filesystem;

//-------------------------------------------------------------------------
// This is a cache of .shader files, so we don't have to load them over and over.
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
std::string find_source(fw::xml_element const &root_elem, std::string source_name);

std::string find_source(fw::xml_element const &root_elem, std::string source_name) {
  for (fw::xml_element child_elem = root_elem.get_first_child();
      child_elem.is_valid(); child_elem = child_elem.get_next_sibling()) {
    if (child_elem.get_name() == "source" && child_elem.get_attribute("name") == source_name) {
      return "#version 330\n\n" + child_elem.get_text();
    }
  }
  BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("No source named '" + source_name + "' found."));
  return ""; // Just to shut up the warning.
}

void compile_shader(GLuint shader_id, std::string source) {
  char const *shader_str = source.c_str();
  FW_CHECKED(glShaderSource(shader_id, 1, &shader_str, nullptr));
  FW_CHECKED(glCompileShader(shader_id));

  GLint status;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    GLint log_length;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> error_message(log_length);
    glGetShaderInfoLog(shader_id, log_length, nullptr, &error_message[0]);

    std::stringstream msg;
    msg << "Error: " << error_message.data() << std::endl;
    msg << "Source:" << std::endl << source << std::endl;
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(msg.str()));
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

namespace fw {
//-------------------------------------------------------------------------
/**
 * A shader_program represents the details of a shader program within a .shader file.
 * It refers to the fragment/vertex shader code + OpenGL states they correspond to.
 */
class shader_program {
private:
  friend class fw::shader_parameters;

  std::string _name;
  std::map<std::string, std::string> _states;
  GLuint _program_id;
  std::map<std::string, fw::shader_variable> _shader_variables;

  /**
   * Called during begin to set the given GL state to the given value.
   *
   * The names and values of the states are just strings, which we need to translate into actual GL function calls.
   */
  void apply_state(std::string const &name, std::string const &value);

public:
  shader_program(fw::xml_element &program_elem);
  ~shader_program();

  void begin();
};

shader_program::shader_program(fw::xml_element &program_elem) : _program_id(0) {
  GLint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
  GLint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
  _program_id = glCreateProgram();

  std::string vertex_shader_source;
  std::string fragment_shader_source;
  for (fw::xml_element child_elem = program_elem.get_first_child();
      child_elem.is_valid(); child_elem = child_elem.get_next_sibling()) {
    if (child_elem.get_name() == "vertex-shader") {
      vertex_shader_source = find_source(program_elem.get_root(), child_elem.get_attribute("source"));
    } else if (child_elem.get_name() == "fragment-shader") {
      fragment_shader_source = find_source(program_elem.get_root(), child_elem.get_attribute("source"));
    } else if (child_elem.get_name() == "state") {
      _states[child_elem.get_attribute("name")] = child_elem.get_attribute("value");
    }
  }
  compile_shader(vertex_shader_id, vertex_shader_source);
  compile_shader(fragment_shader_id, fragment_shader_source);
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
    _shader_variables[name] = fw::shader_variable(location, name, size, type);
  }
}

shader_program::~shader_program() {
}

void shader_program::begin() {
  FW_CHECKED(glUseProgram(_program_id));

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

  BOOST_FOREACH(auto it, _states) {
    apply_state(it.first, it.second);
  }
}

void shader_program::apply_state(std::string const &name, std::string const &value) {
  // TODO: we could probably do something better than this (e.g. at load time rather than
  // at run time)
  if (name == "z-write") {
    if (value == "on") {
      FW_CHECKED(glDepthMask(GL_TRUE));
    } else {
      FW_CHECKED(glDepthMask(GL_FALSE));
    }
  } else if (name == "z-test") {
    if (value == "on") {
      FW_CHECKED(glEnable(GL_DEPTH_TEST));
      FW_CHECKED(glDepthFunc(GL_LEQUAL));
    } else {
      FW_CHECKED(glDisable(GL_DEPTH_TEST));
    }
  } else if (name == "blend") {
    if (value == "alpha") {
      FW_CHECKED(glEnable(GL_BLEND));
      FW_CHECKED(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    } else if (value == "additive") {
      FW_CHECKED(glEnable(GL_BLEND));
      FW_CHECKED(glBlendFunc(GL_ONE, GL_ONE));
    } else {
      FW_CHECKED(glDisable(GL_BLEND));
    }
  }
}

//-------------------------------------------------------------------------
shader_parameters::shader_parameters() {
}

shader_parameters::~shader_parameters() {
}

void shader_parameters::set_program_name(std::string const &name) {
  _program_name = name;
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

void shader_parameters::apply(shader_program *prog) const {
  int texture_unit = 0;
  for (auto it = _textures.begin(); it != _textures.end(); ++it) {
    shader_variable const &var = prog->_shader_variables[it->first];
    if (var.valid) {
      FW_CHECKED(glActiveTexture(GL_TEXTURE0 + texture_unit));
      std::shared_ptr<fw::texture> texture = it->second;
      texture->bind();
      FW_CHECKED(glUniform1i(var.location, texture_unit));
      texture_unit ++;
    }
  }

  for (std::map<std::string, matrix>::const_iterator it = _matrices.begin(); it != _matrices.end(); ++it) {
    shader_variable const &var = prog->_shader_variables[it->first];
    if (var.valid) {
      FW_CHECKED(glUniformMatrix4fv(var.location, 1, GL_FALSE, it->second.data()));
    }
  }

  for (std::map<std::string, vector>::const_iterator it = _vectors.begin(); it != _vectors.end(); ++it) {
    shader_variable const &var = prog->_shader_variables[it->first];
    if (var.valid) {
      FW_CHECKED(glUniform3fv(var.location, 1, it->second.data()));
    }
  }

  for (std::map<std::string, colour>::const_iterator it = _colours.begin(); it != _colours.end(); ++it) {
    shader_variable const &var = prog->_shader_variables[it->first];
    if (var.valid) {
      FW_CHECKED(glUniform4f(var.location, it->second.r, it->second.g, it->second.b, it->second.a));
    }
  }

  for (std::map<std::string, float>::const_iterator it = _scalars.begin(); it != _scalars.end(); ++it) {
    shader_variable const &var = prog->_shader_variables[it->first];
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
shader::shader() {
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
  shader_program *prog;
  if (!parameters || parameters->_program_name == "") {
    prog = _programs[_default_program_name];
  } else {
    prog = _programs[parameters->_program_name];
  }
  prog->begin();
  if (parameters) {
    parameters->apply(prog);
  }
}

void shader::end() {
  FW_CHECKED(glUseProgram(0));
}

std::shared_ptr<shader_parameters> shader::create_parameters() {
  std::shared_ptr<shader_parameters> params(new shader_parameters());
  return params;
}

void shader::load(fw::graphics *g, fs::path const &full_path) {
  _filename = full_path;
  xml_element root_elem = fw::load_xml(full_path, "shader", 1);
  for (xml_element child = root_elem.get_first_child();
      child.is_valid(); child = child.get_next_sibling()) {
    if (child.get_name() != "program") {
      continue;
    }
    shader_program *program = new shader_program(child);
    std::string program_name = child.get_attribute("name");
    if (_default_program_name == "") {
      _default_program_name = program_name;
    }
    _programs[program_name] = program;
  }
}

}

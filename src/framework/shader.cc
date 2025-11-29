#include <framework/shader.h>

#include <filesystem>
#include <string>
#include <sstream>
#include <map>

#include <boost/algorithm/string.hpp>

#include <framework/color.h>
#include <framework/exception.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/logging.h>
#include <framework/math.h>
#include <framework/misc.h>
#include <framework/paths.h>
#include <framework/scenegraph.h>
#include <framework/settings.h>
#include <framework/texture.h>
#include <framework/xml.h>

namespace fs = std::filesystem;

//-------------------------------------------------------------------------
// This is a cache of .Shader files, so we don't have to load them over and over.
class ShaderCache {
private:
  typedef std::map<fs::path, std::shared_ptr<fw::Shader> > shader_map;
  shader_map shaders_;

public:
  std::shared_ptr<fw::Shader> get_shader(fs::path const &name);
  void add_shader(fs::path const &name, std::shared_ptr<fw::Shader> data);

  void clear_cache();
};

std::shared_ptr<fw::Shader> ShaderCache::get_shader(fs::path const &name) {
  shader_map::iterator it = shaders_.find(name);
  if (it == shaders_.end())
    return std::shared_ptr<fw::Shader>();

  return it->second;
}

void ShaderCache::add_shader(fs::path const &name, std::shared_ptr<fw::Shader> data) {
  shaders_[name] = data;
}

void ShaderCache::clear_cache() {
  shaders_.clear();
}

//-----------------------------------------------------------------------------

namespace {

ShaderCache g_cache;
void compile_shader(GLuint shader_id, std::string filename);
void link_shader(GLuint program_id, GLuint vertex_shader_id, GLuint fragment_shader_id);
std::string find_source(fw::XmlElement const &root_elem, std::string source_name);
std::string process_includes(std::string source);
std::string load_include(std::string const &file, std::string const &function_name);

std::string find_source(fw::XmlElement const &root_elem, std::string source_name) {
  for (fw::XmlElement child_elem = root_elem.get_first_child();
      child_elem.is_valid(); child_elem = child_elem.get_next_sibling()) {
    if (child_elem.get_name() == "source" && child_elem.get_attribute("name") == source_name) {
      return "#version 330\n\n" + process_includes(child_elem.get_text());
    }
  }
  BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("No source named '" + source_name + "' found."));
  return ""; // Just to shut up the warning.
}

std::string process_includes(std::string source) {
  std::istringstream ins(source);
  std::string line;
  std::ostringstream outs;

  while(std::getline(ins, line)) {
     boost::trim(line);
     if (line.find("#include") == 0) {
       std::vector<std::string> tokens;
       boost::split(tokens, line, boost::is_any_of(" "));
       if (tokens.size() == 3) {
         std::string file = tokens[1];
         boost::trim_if(file, boost::is_any_of("\""));
         std::string function_name = tokens[2];
         line = load_include(file, function_name);
       }
     }
     outs << line << std::endl;
  }
  return outs.str();
}

std::string load_include(std::string const &file, std::string const &function_name) {
  fw::XmlElement root_elem = fw::load_xml(fw::resolve("shaders/" + file), "shader", 1);
  for (fw::XmlElement child = root_elem.get_first_child();
      child.is_valid(); child = child.get_next_sibling()) {
    if (child.get_name() == "function" && child.get_attribute("name") == function_name) {
      return process_includes(child.get_text());
    }
  }
  return "";
}

void compile_shader(GLuint shader_id, std::string source) {
  char const *shader_str = source.c_str();
  glShaderSource(shader_id, 1, &shader_str, nullptr);
  glCompileShader(shader_id);

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
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info(msg.str()));
  }
}

void link_shader(GLuint program_id, GLuint vertex_shader_id, GLuint fragment_shader_id) {
  glAttachShader(program_id, vertex_shader_id);
  glAttachShader(program_id, fragment_shader_id);
  glLinkProgram(program_id);

  GLint status;
  glGetProgramiv(program_id, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    GLint log_length;
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> error_message(log_length);
    glGetProgramInfoLog(program_id, log_length, nullptr, &error_message[0]);

    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info(std::string(&error_message[0])));
  }
}

} // namespace

namespace fw {
//-------------------------------------------------------------------------
// A ShaderProgram represents the details of a Shader program within a .Shader file.
// It refers to the fragment/vertex Shader code + OpenGL states they correspond to.
class ShaderProgram {
private:
  friend class fw::ShaderParameters;

  std::string name_;
  std::map<std::string, std::string> states_;
  GLuint _program_id;
  std::map<std::string, fw::ShaderVariable> _shader_variables;

  /**
   * Called during begin to set the given GL state to the given ParticleRotation.
   *
   * The names and values of the states are just strings, which we need to translate into actual GL function calls.
   */
  void apply_state(std::string const &name, std::string const &ParticleRotation);

public:
  ShaderProgram(fw::XmlElement &program_elem);
  ~ShaderProgram();

  void begin();
};

ShaderProgram::ShaderProgram(fw::XmlElement &program_elem) : _program_id(0) {
  GLint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
  GLint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
  _program_id = glCreateProgram();

  std::string vertex_shader_source;
  std::string fragment_shader_source;
  for (fw::XmlElement child_elem = program_elem.get_first_child();
      child_elem.is_valid(); child_elem = child_elem.get_next_sibling()) {
    if (child_elem.get_name() == "vertex-shader") {
      vertex_shader_source = find_source(program_elem.get_root(), child_elem.get_attribute("source"));
    } else if (child_elem.get_name() == "fragment-shader") {
      fragment_shader_source = find_source(program_elem.get_root(), child_elem.get_attribute("source"));
    } else if (child_elem.get_name() == "state") {
      states_[child_elem.get_attribute("name")] = child_elem.get_attribute("value");
    }
  }
  compile_shader(vertex_shader_id, vertex_shader_source);
  compile_shader(fragment_shader_id, fragment_shader_source);
  link_shader(_program_id, vertex_shader_id, fragment_shader_id);

  int num_uniforms;
  glGetProgramiv(_program_id, GL_ACTIVE_UNIFORMS, &num_uniforms);
  GLchar buffer[1024];
  for (int i = 0; i < num_uniforms; i++) {
    GLint size;
    GLint length;
    GLenum type;
    glGetActiveUniform(_program_id, i, sizeof(buffer), &size, &length, &type, buffer);
    GLint location = glGetUniformLocation(_program_id, buffer);
    std::string name(buffer);
    _shader_variables[name] = fw::ShaderVariable(location, name, size, type);
  }
}

ShaderProgram::~ShaderProgram() {
}

void ShaderProgram::begin() {
  glUseProgram(_program_id);

#ifdef DEBUG
  GLint status;
  glValidateProgram(_program_id);
  glGetProgramiv(_program_id, GL_VALIDATE_STATUS, &status);
  if (status != GL_TRUE) {
    GLint log_length;
    glGetProgramiv(_program_id, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> error_message(log_length);
    glGetProgramInfoLog(_program_id, log_length, nullptr, &error_message[0]);

    fw::debug << "glValidateProgram error: " << &error_message[0] << std::endl;
  }
#endif

  for(auto it : states_) {
    apply_state(it.first, it.second);
  }
}

void ShaderProgram::apply_state(std::string const &name, std::string const &ParticleRotation) {
  // TODO: we could probably do something better than this (e.g. at load time rather than at run time)
  if (name == "z-write") {
    if (ParticleRotation == "on") {
      glDepthMask(GL_TRUE);
    } else {
      glDepthMask(GL_FALSE);
    }
  } else if (name == "z-test") {
    if (ParticleRotation == "on") {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
    } else {
      glDisable(GL_DEPTH_TEST);
    }
  } else if (name == "blend") {
    if (ParticleRotation == "alpha") {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else if (ParticleRotation == "additive") {
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE);
    } else {
      glDisable(GL_BLEND);
    }
  }
}

//-------------------------------------------------------------------------
ShaderParameters::ShaderParameters() {
}

ShaderParameters::~ShaderParameters() {
}

void ShaderParameters::set_program_name(std::string const &name) {
  program_name_ = name;
}

void ShaderParameters::set_texture(std::string const &name, std::shared_ptr<Texture> const &tex) {
  textures_[name] = tex;
}

void ShaderParameters::set_texture(std::string const& name, std::shared_ptr<TextureArray> const& tex) {
  textures_[name] = tex;
}

void ShaderParameters::set_matrix(std::string const &name, Matrix const &m) {
  matrices_[name] = m;
}

void ShaderParameters::set_vector(std::string const &name, Vector const &v) {
  vectors_[name] = v;
}

void ShaderParameters::set_color(std::string const &name, Color const &c) {
  colors_[name] = c;
}

void ShaderParameters::set_scalar(std::string const &name, float f) {
  scalars_[name] = f;
}

std::shared_ptr<ShaderParameters> ShaderParameters::clone() {
  std::shared_ptr<ShaderParameters> clone(new ShaderParameters());
  clone->textures_ = textures_;
  clone->matrices_ = matrices_;
  clone->vectors_ = vectors_;
  clone->colors_ = colors_;
  clone->scalars_ = scalars_;
  return clone;
}

void ShaderParameters::apply(ShaderProgram *prog) const {
  int texture_unit = 0;
  for (auto it = textures_.begin(); it != textures_.end(); ++it) {
    ShaderVariable const &var = prog->_shader_variables[it->first];
    if (var.valid) {
      glActiveTexture(GL_TEXTURE0 + texture_unit);
      auto& texture = it->second;
      if(texture) {
        texture->ensure_created();
        texture->bind();
      }
      glUniform1i(var.location, texture_unit);
      texture_unit ++;
    }
  }
  while (texture_unit < 9) {
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, 0);
    texture_unit++;
  }

  for (auto it = matrices_.begin(); it != matrices_.end(); ++it) {
    ShaderVariable const &var = prog->_shader_variables[it->first];
    if (var.valid) {
      glUniformMatrix4fv(var.location, 1, GL_FALSE, it->second.m[0]);
    }
  }

  for (auto it = vectors_.begin(); it != vectors_.end(); ++it) {
    ShaderVariable const &var = prog->_shader_variables[it->first];
    if (var.valid) {
      glUniform3fv(var.location, 1, it->second.v);
    }
  }

  for (auto it = colors_.begin(); it != colors_.end(); ++it) {
    ShaderVariable const &var = prog->_shader_variables[it->first];
    if (var.valid) {
      glUniform4f(var.location, it->second.r, it->second.g, it->second.b, it->second.a);
    }
  }

  for (auto it = scalars_.begin(); it != scalars_.end(); ++it) {
    ShaderVariable const &var = prog->_shader_variables[it->first];
    if (var.valid) {
      glUniform1f(var.location, it->second);
    }
  }
}

//-------------------------------------------------------------------------

ShaderVariable::ShaderVariable() :
  location(-1), name(""), size(0), type(0), valid(false) {
}

ShaderVariable::ShaderVariable(GLint location, std::string name, GLint size, GLenum type) :
  location(location), name(name), size(size), type(type), valid(true) {
}

//-------------------------------------------------------------------------
Shader::Shader() {
}

Shader::~Shader() {
}

std::shared_ptr<Shader> Shader::create(std::string const &filename) {
  fw::Graphics *g = fw::Framework::get_instance()->get_graphics();

  std::shared_ptr<Shader> shdr = g_cache.get_shader(filename);
  if (!shdr) {
    shdr = std::shared_ptr<Shader>(new Shader());
    shdr->load(g, fw::resolve("shaders/" + filename));
    g_cache.add_shader(filename, shdr);
  }

  return shdr;
}

void Shader::begin(std::shared_ptr<ShaderParameters> parameters) {
  ShaderProgram *prog;
  std::string program_name = default_program_name_;
  if (parameters && parameters->program_name_ != "") {
    program_name = parameters->program_name_;
  }
  prog = programs_[program_name];
  if (prog == nullptr) {
    prog = programs_[default_program_name_];
  }
  prog->begin();
  if (parameters) {
    parameters->apply(prog);
  }
}

void Shader::end() {
  glUseProgram(0);
}

std::shared_ptr<ShaderParameters> Shader::create_parameters() {
  std::shared_ptr<ShaderParameters> params(new ShaderParameters());
  return params;
}

void Shader::load(fw::Graphics *g, fs::path const &full_path) {
  filename_ = full_path;
  XmlElement root_elem = fw::load_xml(full_path, "shader", 1);
  for (XmlElement child = root_elem.get_first_child();
      child.is_valid(); child = child.get_next_sibling()) {
    if (child.get_name() != "program") {
      continue;
    }
    ShaderProgram *program = new ShaderProgram(child);
    std::string program_name = child.get_attribute("name");
    if (default_program_name_ == "") {
      default_program_name_ = program_name;
    }
    programs_[program_name] = program;
  }
}

}

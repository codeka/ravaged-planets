#include <framework/shader.h>

#include <filesystem>
#include <string>
#include <sstream>
#include <map>

#include <absl/strings/str_cat.h>
#include <absl/strings/str_split.h>

#include <framework/color.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/logging.h>
#include <framework/math.h>
#include <framework/misc.h>
#include <framework/paths.h>
#include <framework/scenegraph.h>
#include <framework/settings.h>
#include <framework/status.h>
#include <framework/texture.h>
#include <framework/xml.h>

namespace fs = std::filesystem;

//-------------------------------------------------------------------------
// This is a cache of .Shader files, so we don't have to load them over and over.
class ShaderCache {
public:
  std::shared_ptr<fw::Shader> GetShader(fs::path const &name);
  void AddShader(fs::path const &name, std::shared_ptr<fw::Shader> data);

  void ClearCache();

private:
  typedef std::map<fs::path, std::shared_ptr<fw::Shader>> shader_map;
  shader_map shaders_;
};

std::shared_ptr<fw::Shader> ShaderCache::GetShader(fs::path const &name) {
  shader_map::iterator it = shaders_.find(name);
  if (it == shaders_.end())
    return std::shared_ptr<fw::Shader>();

  return it->second;
}

void ShaderCache::AddShader(fs::path const &name, std::shared_ptr<fw::Shader> data) {
  shaders_[name] = data;
}

void ShaderCache::ClearCache() {
  shaders_.clear();
}

//-----------------------------------------------------------------------------

namespace {

ShaderCache g_cache;
fw::Status CompileShader(GLuint shader_id, std::string filename);
fw::Status LinkShader(GLuint program_id, GLuint vertex_shader_id, GLuint fragment_shader_id);
fw::StatusOr<std::string> ProcessIncludes(std::string const &source);

fw::StatusOr<std::string> FindSource(fw::XmlElement const &root_elem, std::string source_name) {
  for (fw::XmlElement child_elem : root_elem.children()) {
    ASSIGN_OR_RETURN(auto child_name, child_elem.GetAttribute("name"));
    if (child_elem.get_name() == "source" && child_name == source_name) {
      ASSIGN_OR_RETURN(auto include_text, ProcessIncludes(child_elem.get_text()));
      return "#version 330\n\n" + include_text;
    }
  }

  return fw::ErrorStatus(absl::StrCat("no source named '", source_name, "' found."));
}


fw::StatusOr<std::string> LoadInclude(
    std::filesystem::path const &file, std::string_view function_name) {
  ASSIGN_OR_RETURN(
      fw::XmlElement root_elem, fw::LoadXml(fw::resolve("shaders") / file, "shader", 1));
  for (fw::XmlElement child : root_elem.children()) {
    if (child.get_name() == "function") {
      ASSIGN_OR_RETURN(auto name, child.GetAttribute("name"));
      if (name == function_name) {
        return ProcessIncludes(child.get_text());
      }
    }
  }
  return std::string("");
}

fw::StatusOr<std::string> ProcessIncludes(std::string const &source) {
  std::istringstream ins(source);
  std::string line;
  std::ostringstream outs;

  while(std::getline(ins, line)) {
     line = fw::StripSpaces(line);
     if (line.find("#include") == 0) {
       std::vector<std::string> tokens = absl::StrSplit(line, " ");
       if (tokens.size() == 3) {
         std::string file = tokens[1];
         file = fw::Strip(file, '\"');
         std::string function_name = tokens[2];
         ASSIGN_OR_RETURN(line, LoadInclude(file, function_name));
       }
     }
     outs << line << std::endl;
  }
  return outs.str();
}


fw::Status CompileShader(GLuint shader_id, std::string source) {
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
    msg << "failed to compile shader: " << error_message.data() << std::endl;
    msg << "   source:" << std::endl << source << std::endl;
    return fw::ErrorStatus(msg.str());
  }
  
  return fw::OkStatus();
}

fw::Status LinkShader(GLuint program_id, GLuint vertex_shader_id, GLuint fragment_shader_id) {
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

    return fw::ErrorStatus("failed to link shader: ") << &error_message[0];
  }

  return fw::OkStatus();
}

} // namespace

namespace fw {
//-------------------------------------------------------------------------
// A ShaderProgram represents the details of a Shader program within a .shader file.
// It refers to the fragment/vertex shader code + OpenGL states they correspond to.
class ShaderProgram {
private:
  friend class fw::ShaderParameters;

  std::string name_;
  std::map<std::string, std::string> states_;
  GLuint program_id_;
  std::map<std::string, fw::ShaderVariable> shader_variables_;

  /**
   * Called during begin to set the given GL state to the given value.
   *
   * The names and values of the states are just strings, which we need to translate into actual GL
   * function calls.
   */
  void ApplyState(std::string_view name, std::string_view value);

public:
  ShaderProgram() = default;
  ~ShaderProgram() = default;

  fw::Status Initialize(fw::XmlElement const &program_elem);

  void Begin();
};

fw::Status ShaderProgram::Initialize(fw::XmlElement const &program_elem) {
  GLint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
  GLint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
  program_id_ = glCreateProgram();

  std::string vertex_shader_source;
  std::string fragment_shader_source;
  for (fw::XmlElement child_elem : program_elem.children()) {
    if (child_elem.get_name() == "vertex-shader") {
      ASSIGN_OR_RETURN(auto source, child_elem.GetAttribute("source"));
      ASSIGN_OR_RETURN(vertex_shader_source, FindSource(program_elem.get_root(), source));
    } else if (child_elem.get_name() == "fragment-shader") {
      ASSIGN_OR_RETURN(auto source, child_elem.GetAttribute("source"));
      ASSIGN_OR_RETURN(fragment_shader_source, FindSource(program_elem.get_root(), source));
    } else if (child_elem.get_name() == "state") {
      ASSIGN_OR_RETURN(auto name, child_elem.GetAttribute("name"));
      ASSIGN_OR_RETURN(auto value, child_elem.GetAttribute("value"));
      states_[name] = value;
    }
  }
  RETURN_IF_ERROR(CompileShader(vertex_shader_id, vertex_shader_source));
  RETURN_IF_ERROR(CompileShader(fragment_shader_id, fragment_shader_source));
  RETURN_IF_ERROR(LinkShader(program_id_, vertex_shader_id, fragment_shader_id));

  int num_uniforms;
  glGetProgramiv(program_id_, GL_ACTIVE_UNIFORMS, &num_uniforms);
  GLchar buffer[1024];
  for (int i = 0; i < num_uniforms; i++) {
    GLint size;
    GLint length;
    GLenum type;
    glGetActiveUniform(program_id_, i, sizeof(buffer), &size, &length, &type, buffer);
    GLint location = glGetUniformLocation(program_id_, buffer);
    std::string name(buffer);
    shader_variables_[name] = fw::ShaderVariable(location, name, size, type);
  }

  return fw::OkStatus();
}

void ShaderProgram::Begin() {
  glUseProgram(program_id_);

#ifdef DEBUG
  GLint status;
  glValidateProgram(program_id_);
  glGetProgramiv(program_id_, GL_VALIDATE_STATUS, &status);
  if (status != GL_TRUE) {
    GLint log_length;
    glGetProgramiv(program_id_, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> error_message(log_length);
    glGetProgramInfoLog(program_id_, log_length, nullptr, &error_message[0]);

    fw::debug << "glValidateProgram error: " << &error_message[0] << std::endl;
  }
#endif

  for(auto it : states_) {
    ApplyState(it.first, it.second);
  }
}

void ShaderProgram::ApplyState(std::string_view name, std::string_view value) {
  // TODO: we could probably do something better than this (e.g. at load time rather than at run
  // time)
  if (name == "z-write") {
    if (value == "on") {
      glDepthMask(GL_TRUE);
    } else {
      glDepthMask(GL_FALSE);
    }
  } else if (name == "z-test") {
    if (value == "on") {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
    } else {
      glDisable(GL_DEPTH_TEST);
    }
  } else if (name == "blend") {
    if (value == "alpha") {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else if (value == "additive") {
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

void ShaderParameters::set_program_name(std::string_view name) {
  program_name_ = name;
}

void ShaderParameters::set_texture(std::string_view name, std::shared_ptr<Texture> const &tex) {
  textures_[std::string(name)] = tex;
}

void ShaderParameters::set_texture(std::string_view name, std::shared_ptr<TextureArray> const& tex) {
  textures_[std::string(name)] = tex;
}

void ShaderParameters::set_matrix(std::string_view name, Matrix const &m) {
  matrices_[std::string(name)] = m;
}

void ShaderParameters::set_vector(std::string_view name, Vector const &v) {
  vectors_[std::string(name)] = v;
}

void ShaderParameters::set_color(std::string_view name, Color const &c) {
  colors_[std::string(name)] = c;
}

void ShaderParameters::set_scalar(std::string_view name, float f) {
  scalars_[std::string(name)] = f;
}

std::shared_ptr<ShaderParameters> ShaderParameters::Clone() {
  auto clone = std::make_shared<ShaderParameters>();
  clone->textures_ = textures_;
  clone->matrices_ = matrices_;
  clone->vectors_ = vectors_;
  clone->colors_ = colors_;
  clone->scalars_ = scalars_;
  return clone;
}

void ShaderParameters::Apply(ShaderProgram &prog) const {
  int texture_unit = 0;
  for (auto it = textures_.begin(); it != textures_.end(); ++it) {
    ShaderVariable const &var = prog.shader_variables_[it->first];
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
    ShaderVariable const &var = prog.shader_variables_[it->first];
    if (var.valid) {
      glUniformMatrix4fv(var.location, 1, GL_FALSE, it->second.m[0]);
    }
  }

  for (auto it = vectors_.begin(); it != vectors_.end(); ++it) {
    ShaderVariable const &var = prog.shader_variables_[it->first];
    if (var.valid) {
      glUniform3fv(var.location, 1, it->second.v);
    }
  }

  for (auto it = colors_.begin(); it != colors_.end(); ++it) {
    ShaderVariable const &var = prog.shader_variables_[it->first];
    if (var.valid) {
      glUniform4f(var.location, it->second.r, it->second.g, it->second.b, it->second.a);
    }
  }

  for (auto it = scalars_.begin(); it != scalars_.end(); ++it) {
    ShaderVariable const &var = prog.shader_variables_[it->first];
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

/*static*/
fw::StatusOr<std::shared_ptr<Shader>> Shader::Create(std::string_view filename) {
  fw::Graphics *g = fw::Framework::get_instance()->get_graphics();

  auto shader = g_cache.GetShader(filename);
  if (!shader) {
    shader = std::make_shared<Shader>();
    RETURN_IF_ERROR(shader->Load(g, fw::resolve(absl::StrCat("shaders/", filename))));
    g_cache.AddShader(filename, shader);
  }

  return shader;
}

/*static*/
std::shared_ptr<Shader> Shader::CreateOrEmpty(std::string_view name) {
  auto shader = Create(name);
  if (!shader.ok()) {
    fw::debug << "ERROR creating shader '" << name << "': " << shader.status() << std::endl;
    return std::make_shared<Shader>();
  }
  return *shader;
}

void Shader::Begin(std::shared_ptr<ShaderParameters> parameters) {
  std::string program_name = default_program_name_;
  if (parameters && parameters->program_name_ != "") {
    program_name = parameters->program_name_;
  }
  auto prog = programs_[program_name];
  if (!prog) {
    prog = programs_[default_program_name_];
  }
  prog->Begin();
  if (parameters) {
    parameters->Apply(*prog);
  }
}

void Shader::End() {
  glUseProgram(0);
}

std::shared_ptr<ShaderParameters> Shader::CreateParameters() {
  return std::make_shared<ShaderParameters>();
}

fw::Status Shader::Load(fw::Graphics *g, fs::path const &full_path) {
  filename_ = full_path;
  ASSIGN_OR_RETURN(XmlElement root_elem, fw::LoadXml(full_path, "shader", 1));
  for (XmlElement child : root_elem.children()) {
    if (child.get_name() != "program") {
      continue;
    }

    auto program = std::make_shared<ShaderProgram>();
    RETURN_IF_ERROR(program->Initialize(child));
    ASSIGN_OR_RETURN(std::string program_name, child.GetAttribute("name"));
    if (default_program_name_ == "") {
      default_program_name_ = program_name;
    }
    programs_[program_name] = program;
  }
  return fw::OkStatus();
}

}

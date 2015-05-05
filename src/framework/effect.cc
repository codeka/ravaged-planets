#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <framework/effect.h>
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

namespace fs = boost::filesystem;

static std::map<fw::sg::primitive_type, uint32_t> g_primitive_type_map;

static void ensure_primitive_type_map() {
  if (g_primitive_type_map.size() > 0)
    return;

  g_primitive_type_map[fw::sg::primitive_linestrip] = GL_LINE_STRIP;
  g_primitive_type_map[fw::sg::primitive_linelist] = GL_LINES;
  g_primitive_type_map[fw::sg::primitive_trianglelist] = GL_TRIANGLES;
  g_primitive_type_map[fw::sg::primitive_trianglestrip] = GL_TRIANGLE_STRIP;
}

struct effect_data: boost::noncopyable {
  fs::path filename;
  GLuint program_id;

  GLint worldviewproj_location;
  GLint position_location;

  inline ~effect_data() {
    fw::debug << "effect_data::~effect_data()" << std::endl;
    glDeleteProgram(program_id);
  }
};

//-------------------------------------------------------------------------
// This is a cache of .fx files, so we don't have to load them over and over...
class effect_cache {
private:
  typedef std::map<fs::path, std::shared_ptr<effect_data> > effect_map;
  effect_map _effects;

public:
  std::shared_ptr<effect_data> get_effect(fs::path const &name);
  void add_effect(fs::path const &name, std::shared_ptr<effect_data> data);

  void clear_cache();
};

std::shared_ptr<effect_data> effect_cache::get_effect(fs::path const &name) {
  effect_map::iterator it = _effects.find(name);
  if (it == _effects.end())
    return std::shared_ptr<effect_data>();

  return it->second;
}

void effect_cache::add_effect(fs::path const &name, std::shared_ptr<effect_data> data) {
  _effects[name] = data;
}

void effect_cache::clear_cache() {
  _effects.clear();
}

//-----------------------------------------------------------------------------

namespace {
effect_cache g_cache;
std::shared_ptr<effect_data> load_effect(fw::graphics *g, fs::path const &full_path);
}

namespace fw {
//-------------------------------------------------------------------------
effect_parameters::effect_parameters() {
}

effect_parameters::~effect_parameters() {
}

void effect_parameters::set_texture(std::string const &name, std::shared_ptr<texture> const &tex) {
  _textures[name] = tex;
}

void effect_parameters::set_vertex_buffer(std::string const &name, std::shared_ptr<vertex_buffer> const &vb) {
  _vertex_buffers[name] = vb;
}

void effect_parameters::set_matrix(std::string const &name, matrix const &m) {
  _matrices[name] = m;
}

void effect_parameters::set_vector(std::string const &name, vector const &v) {
  _vectors[name] = v;
}

void effect_parameters::set_colour(std::string const &name, colour const &c) {
  _colours[name] = c;
}

void effect_parameters::set_scalar(std::string const &name, float f) {
  _scalars[name] = f;
}

std::shared_ptr<effect_parameters> effect_parameters::clone() {
  std::shared_ptr<effect_parameters> clone(new effect_parameters());
  clone->_textures = _textures;
  clone->_matrices = _matrices;
  clone->_vectors = _vectors;
  clone->_colours = _colours;
  clone->_scalars = _scalars;
  return clone;
}

void effect_parameters::apply(effect *e) const {
  if (e->_data == 0)
    return;

  for (auto it = _textures.begin(); it != _textures.end(); ++it) {
    GLint id = glGetUniformLocation(e->_data->program_id, it->first.c_str());
//    glTexture();
  }

  for (auto it = _vertex_buffers.begin(); it != _vertex_buffers.end(); ++it) {
    if (it->first == "position") {
      it->second->bind(e->_data->position_location);
      continue;
    }

    GLint id = glGetAttribLocation(e->_data->program_id, it->first.c_str());
    if (id > 0) {
      it->second->bind(id);
    } else {
      fw::debug << "Warning: No location for '" << it->first.c_str() << "' in " << e->_data->filename.filename()
          << std::endl;
    }
  }

  for (std::map<std::string, matrix>::const_iterator it = _matrices.begin(); it != _matrices.end(); ++it) {
    if (it->first == "worldviewproj") {
      FW_CHECKED(glUniformMatrix4fv(e->_data->worldviewproj_location, 1, GL_FALSE, it->second.data()));
      continue;
    }

    GLint id = glGetUniformLocation(e->_data->program_id, it->first.c_str());
    if (id > 0) {
      FW_CHECKED(glUniformMatrix4fv(id, 1, GL_FALSE, it->second.data()));
    } else {
     // fw::debug << "Warning: No location for '" << it->first.c_str() << "' in " << e->_data->filename.filename()
     //     << std::endl;
    }
  }

  for (std::map<std::string, vector>::const_iterator it = _vectors.begin(); it != _vectors.end(); ++it) {
//    GLint id = glGetAttribLocation(e->_data->program_id, it->first.c_str());
//    if (id > 0) {
//      FW_CHECKED(glEnableVertexAttribArray(id));
//      FW_CHECKED()
//      FW_CHECKED(glVertexAttribPointer(id, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr));
//    }
    //CGparameter p = cgGetNamedEffectParameter(fx, it->first.c_str());
    //if (p != 0) {
    //  CG_CHECKED(cgSetParameter3fv(p, it->second.data()));
    //}
  }

  for (std::map<std::string, colour>::const_iterator it = _colours.begin(); it != _colours.end(); ++it) {
    //CGparameter p = cgGetNamedEffectParameter(fx, it->first.c_str());
    //if (p != 0) {
    //  CG_CHECKED(
    //      cgSetParameter4f(p, it->second.a, it->second.r, it->second.g,
    //           it->second.b));
    //}
  }

  for (std::map<std::string, float>::const_iterator it = _scalars.begin(); it != _scalars.end(); ++it) {
    //CGparameter p = cgGetNamedEffectParameter(fx, it->first.c_str());
    //if (p != 0) {
    //  CG_CHECKED(cgSetParameter1f(p, it->second));
    //}
  }

}

//-------------------------------------------------------------------------
effect::effect() {
}

effect::~effect() {
}

void effect::initialise(fs::path const &filename) {
  fw::graphics *g = fw::framework::get_instance()->get_graphics();

  _data = g_cache.get_effect(filename);
  if (!_data) {
    _data = load_effect(g, fw::resolve("share/ravaged-planets/shaders/" + filename.string()));
    g_cache.add_effect(filename, _data);
  }
}

void effect::render(std::shared_ptr<effect_parameters> parameters, fw::sg::primitive_type primitive_type,
    index_buffer *idx_buffer) {
  if (!_data)
    return;
  ensure_primitive_type_map();

  FW_CHECKED(glUseProgram(_data->program_id));
  if (parameters) {
    parameters->apply(this);
  }

  idx_buffer->prepare();
/*
#ifdef DEBUG
  GLint status;
  FW_CHECKED(glValidateProgram(_data->program_id));
  glGetProgramiv(_data->program_id, GL_VALIDATE_STATUS, &status);
  if (status != GL_TRUE) {
    GLint log_length;
    glGetProgramiv(_data->program_id, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> error_message(log_length);
    glGetProgramInfoLog(_data->program_id, log_length, nullptr, &error_message[0]);

    fw::debug << "glValidateProgram error: " << &error_message[0] << std::endl;
  }
#endif
*/
  FW_CHECKED(glDrawElements(g_primitive_type_map[primitive_type], idx_buffer->get_num_indices(),
      GL_UNSIGNED_SHORT, nullptr));
}

std::shared_ptr<effect_parameters> effect::create_parameters() {
  std::shared_ptr<effect_parameters> params(new effect_parameters());
  return params;
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
/*
  FW_CHECKED(glBindAttribLocation(program_id, 0, "position"));
  FW_CHECKED(glBindAttribLocation(program_id, 1, "normal"));
*/
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

 // FW_CHECKED(glDetachShader(program_id, vertex_shader_id));
 // FW_CHECKED(glDetachShader(program_id, fragment_shader_id));
}

std::shared_ptr<effect_data> load_effect(fw::graphics *g, fs::path const &full_path) {
  std::shared_ptr<effect_data> data(new effect_data());
  data->filename = full_path;
  GLint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
  GLint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
  data->program_id = glCreateProgram();

  compile_shader(vertex_shader_id, full_path.string() + ".vert");
  compile_shader(fragment_shader_id, full_path.string() + ".frag");
  link_shader(data->program_id, vertex_shader_id, fragment_shader_id);

 // FW_CHECKED(glDeleteShader(vertex_shader_id));
 // FW_CHECKED(glDeleteShader(fragment_shader_id));

  data->worldviewproj_location = glGetUniformLocation(data->program_id, "worldviewproj");
  if (data->worldviewproj_location < 0) {
//    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("No location for worldviewproj"));
  }

  data->position_location = glGetAttribLocation(data->program_id, "position");
  if (data->position_location < 0) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("No location for position"));
  }

  fw::debug << "loaded effect from: " << full_path.string() << std::endl;
  return data;
}

}

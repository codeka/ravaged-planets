#include <memory>

#include <stb/stb_image.h>

#include <framework/texture.h>
#include <framework/framework.h>
#include <framework/bitmap.h>
#include <framework/graphics.h>
#include <framework/misc.h>
#include <framework/paths.h>
#include <framework/logging.h>
#include <framework/exception.h>

namespace fs = boost::filesystem;

namespace fw {

//-------------------------------------------------------------------------
struct texture_data: private boost::noncopyable {
  GLuint texture_id;
  int width, height;
  fs::path filename; // might be empty if we weren't created from a file

  texture_data() : texture_id(0), width(-1), height(-1) {
    FW_CHECKED(glGenTextures(1, &texture_id));
  }

  ~texture_data() {
    FW_CHECKED(glDeleteTextures(1, &texture_id));
  }
};

struct framebuffer_data: private boost::noncopyable {
  GLuint fbo_id;
  std::shared_ptr<texture> colour_texture;
  std::shared_ptr<texture> depth_texture;

  framebuffer_data() {
    FW_CHECKED(glGenFramebuffers(1, &fbo_id));
  }

  ~framebuffer_data() {
    FW_CHECKED(glDeleteFramebuffers(1, &fbo_id));
  }
};

//-------------------------------------------------------------------------
// This is a cache of textures, so we don't have to load them over and over...
class texture_cache {
private:
  typedef std::map<boost::filesystem::path, std::shared_ptr<texture_data> > texture_map;
  texture_map _textures;

public:
  std::shared_ptr<texture_data> get_texture(fs::path const &filename);
  void add_texture(fs::path const &filename, std::shared_ptr<texture_data> data);

  void clear_cache();
};

std::shared_ptr<texture_data> texture_cache::get_texture(fs::path const &filename) {
  texture_map::iterator it = _textures.find(filename);
  if (it == _textures.end())
    return std::shared_ptr<texture_data>();

  return it->second;
}

void texture_cache::add_texture(fs::path const &filename, std::shared_ptr<texture_data> data) {
  _textures[filename] = data;
}

void texture_cache::clear_cache() {
  _textures.clear();
}

static texture_cache g_cache;

//-------------------------------------------------------------------------

texture::texture() {
}

texture::~texture() {
}

void texture::create(fs::path const &filename) {
  graphics *g = fw::framework::get_instance()->get_graphics();

  _data = g_cache.get_texture(filename);
  if (!_data) {
    std::shared_ptr<texture_data> data(new texture_data());
    _data = data;

    debug << boost::format("loading texture: %1%") % filename << std::endl;
    FW_CHECKED(glBindTexture(GL_TEXTURE_2D, _data->texture_id));

    _data->filename = filename;
    int channels;
    unsigned char *pixels = stbi_load(filename.string().c_str(), &_data->width, &_data->height, &channels, 4);
    // TODO: pre-multiply alpha
    // TODO: DXT compress
    // TODO: mipmaps
    FW_CHECKED(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _data->width, _data->height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, pixels));
    stbi_image_free(pixels);

    g_cache.add_texture(filename, _data);
  }
}

void texture::create(std::shared_ptr<fw::bitmap> bmp) {
  create(*bmp);
}

void texture::create(fw::bitmap const &bmp) {
  graphics *g = fw::framework::get_instance()->get_graphics();

  if (!_data) {
    _data = std::shared_ptr<texture_data>(new texture_data());
  }
  _data->width = bmp.get_width();
  _data->height = bmp.get_height();

  FW_CHECKED(glBindTexture(GL_TEXTURE_2D, _data->texture_id));
  FW_CHECKED(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _data->width, _data->height, 0, GL_RGBA,
      GL_UNSIGNED_BYTE, bmp.get_pixels().data()));
}

void texture::create(int width, int height, bool is_shadowmap) {
  graphics *g = fw::framework::get_instance()->get_graphics();

  if (!_data) {
    _data = std::shared_ptr<texture_data>(new texture_data());
  }
  _data->width = width;
  _data->height = height;

  FW_CHECKED(glBindTexture(GL_TEXTURE_2D, _data->texture_id));
  if (is_shadowmap) {
    FW_CHECKED(glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, _data->width, _data->height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));
  } else {
    FW_CHECKED(glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, _data->width, _data->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
  }
}

void texture::bind() const {
  if (!_data) {
    FW_CHECKED(glBindTexture(GL_TEXTURE_2D, 0));
    return;
  }

  FW_CHECKED(glBindTexture(GL_TEXTURE_2D, _data->texture_id));
  FW_CHECKED(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  FW_CHECKED(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
}

void texture::save_png(fs::path const &filename) {
  bitmap bmp(*this);
  bmp.save_bitmap(filename);
}

void texture::calculate_size() const {
  if (_data->width > 0 && _data->height > 0)
    return;
}

int texture::get_width() const {
  calculate_size();
  return _data->width;
}

int texture::get_height() const {
  calculate_size();
  return _data->height;
}

fs::path texture::get_filename() const {
  return _data->filename;
}

//-----------------------------------------------------------------------------
framebuffer::framebuffer() : _data(new framebuffer_data()) {
}

framebuffer::~framebuffer() {
}

void framebuffer::set_colour_buffer(std::shared_ptr<texture> colour_texture) {
  _data->colour_texture = colour_texture;
}

void framebuffer::set_depth_buffer(std::shared_ptr<texture> depth_texture) {
  _data->depth_texture = depth_texture;
}

std::shared_ptr<texture> framebuffer::get_colour_buffer() const {
  return _data->colour_texture;
}

std::shared_ptr<texture> framebuffer::get_depth_buffer() const {
  return _data->depth_texture;
}

void framebuffer::bind() {
  FW_CHECKED(glBindFramebuffer(GL_FRAMEBUFFER, _data->fbo_id));
  if (_data->depth_texture) {
    GLuint texture_id = _data->depth_texture->get_data()->texture_id;
    FW_CHECKED(glBindTexture(GL_TEXTURE_2D, texture_id));
    FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    FW_CHECKED(glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture_id, 0));
  }

  if (_data->colour_texture) {
    GLuint texture_id = _data->colour_texture->get_data()->texture_id;
    FW_CHECKED(glBindTexture(GL_TEXTURE_2D, texture_id));
    FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    FW_CHECKED(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_id, 0));
    FW_CHECKED(glDrawBuffer(GL_COLOR_ATTACHMENT0));
  } else {
    FW_CHECKED(glDrawBuffer(GL_NONE));
  }

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    fw::debug << "Framebuffer is not complete, expect errors: " << status << std::endl;
  }
}

void framebuffer::unbind() {
  FW_CHECKED(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  FW_CHECKED(glDrawBuffer(GL_BACK));
}

int framebuffer::get_width() const {
  if (_data->colour_texture) {
    return _data->colour_texture->get_width();
  } else if (_data->depth_texture) {
    return _data->depth_texture->get_width();
  } else {
    return 0;
  }
}

int framebuffer::get_height() const {
  if (_data->colour_texture) {
    return _data->colour_texture->get_height();
  } else if (_data->depth_texture) {
    return _data->depth_texture->get_height();
  } else {
    return 0;
  }
}

}

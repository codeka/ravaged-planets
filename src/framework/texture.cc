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
  bool is_render_target;
  mutable int width, height;
  fs::path filename; // might be empty if we weren't created from a file

  texture_data() : texture_id(0), is_render_target(false), width(-1), height(-1) {
    FW_CHECKED(glGenTextures(1, &texture_id));
  }

  ~texture_data() {
    FW_CHECKED(glDeleteTextures(1, &texture_id));
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

void texture::create(std::shared_ptr<fw::bitmap> bmp, bool dynamic) {
  create(*bmp, dynamic);
}

void texture::create(fw::bitmap const &bmp, bool dynamic) {
  graphics *g = fw::framework::get_instance()->get_graphics();

  if (!_data) {
    _data = std::shared_ptr<texture_data>(new texture_data());
  }
  _data->width = bmp.get_width();
  _data->height = bmp.get_height();

  FW_CHECKED(glBindTexture(GL_TEXTURE_2D, _data->texture_id));
  FW_CHECKED(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _data->width, _data->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
      bmp.get_pixels().data()));
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
/*  HRESULT hr = ::D3DXSaveTextureToFile(wfilename.c_str(), D3DXIFF_PNG,
      _data->tex, 0);
  if (FAILED(hr)) {
    debug
        << boost::format(
            "WARN: could not save texture to file: \"%1%\": 0x%2$x") % filename
            % hr << std::endl;
  }*/
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

}

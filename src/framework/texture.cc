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
struct TextureData {
  GLuint texture_id = 0;
  int width = -1, height = -1;
  fs::path filename; // might be empty if we weren't created from a file

  TextureData() = default;
  TextureData(const TextureData&) = delete;
  TextureData& operator=(const TextureData&) = delete;
};

//-------------------------------------------------------------------------
// This is a cache of textures, so we don't have to load them over and over.
class TextureCache {
private:
  typedef std::map<boost::filesystem::path, std::shared_ptr<TextureData>> TexturesMap;
  TexturesMap textures_;

public:
  std::shared_ptr<TextureData> get_texture(fs::path const &filename);
  void add_texture(fs::path const &filename, std::shared_ptr<TextureData> data);

  void clear_cache();
};

std::shared_ptr<TextureData> TextureCache::get_texture(fs::path const &filename) {
  TexturesMap::iterator it = textures_.find(filename);
  if (it == textures_.end())
    return std::shared_ptr<TextureData>();

  return it->second;
}

void TextureCache::add_texture(fs::path const &filename, std::shared_ptr<TextureData> data) {
  textures_[filename] = data;
}

void TextureCache::clear_cache() {
  FW_ENSURE_RENDER_THREAD();

  for (auto texture : textures_) {
    glDeleteTextures(1, &texture.second->texture_id);
  }
  textures_.clear();
}

static TextureCache g_cache;

//-------------------------------------------------------------------------

Texture::Texture() {
}

Texture::~Texture() {
  // TODO: do I need to glDeleteTextures?
}

void Texture::create(fs::path const &fn) {
  Graphics *g = fw::Framework::get_instance()->get_graphics();

  // Local the image on this thread to avoid loading it on the render thread.
  fs::path filename = fn;
  debug << boost::format("loading texture: %1%") % filename << std::endl;
  int width, height, channels;
  unsigned char* pixels = stbi_load(filename.string().c_str(), &width, &height, &channels, 4);

  data_ = std::make_shared<TextureData>();
  data_->filename = filename;
  data_->width = width;
  data_->height = height;

  data_creator_ = [g, filename, pixels, width, height, channels](TextureData& data) {
    if (data.texture_id == 0) {
      glGenTextures(1, &data.texture_id);
    }
    FW_CHECKED(glBindTexture(GL_TEXTURE_2D, data.texture_id));
    // TODO: pre-multiply alpha
    // TODO: DXT compress
    // TODO: mipmaps
    FW_CHECKED(
      glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, data.width, data.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels));
    stbi_image_free(pixels);
  };
}

void Texture::create(std::shared_ptr<fw::Bitmap> bmp) {
  create(*bmp);
}

void Texture::create(fw::Bitmap const &bmp) {
  Graphics *g = fw::Framework::get_instance()->get_graphics();
  fw::Bitmap bitmap(bmp);

  data_ = std::make_shared<TextureData>();
  data_->width = bitmap.get_width();
  data_->height = bitmap.get_height();

  data_creator_ = [g, bitmap](TextureData& data) {
    if (data.texture_id == 0) {
      glGenTextures(1, &data.texture_id);
    }
    FW_CHECKED(glBindTexture(GL_TEXTURE_2D, data.texture_id));
    FW_CHECKED(
      glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, data.width, data.height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, bitmap.get_pixels().data()));
  };
}

void Texture::create(int width, int height, bool is_shadowmap) {
  Graphics *g = fw::Framework::get_instance()->get_graphics();

  data_ = std::make_shared<TextureData>();
  data_->width = width;
  data_->height = height;

  data_creator_ = [g, width, height, is_shadowmap](TextureData& data) {
    if (data.texture_id == 0) {
      glGenTextures(1, &data.texture_id);
    }
    FW_CHECKED(glBindTexture(GL_TEXTURE_2D, data.texture_id));
    if (is_shadowmap) {
      FW_CHECKED(glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, data.width, data.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));
    } else {
      FW_CHECKED(glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, data.width, data.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
    }
  };
}

void Texture::ensure_created() {
  FW_ENSURE_RENDER_THREAD();
  if (data_creator_) {
    data_creator_(*data_);
    data_creator_ = nullptr;
  }
}

void Texture::bind() const {
  if (!data_) {
    FW_CHECKED(glBindTexture(GL_TEXTURE_2D, 0));
    return;
  }

  FW_CHECKED(glBindTexture(GL_TEXTURE_2D, data_->texture_id));
  FW_CHECKED(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  FW_CHECKED(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
}

void Texture::save_png(fs::path const &filename) {
  Bitmap bmp(*this);
  bmp.save_bitmap(filename);
}

void Texture::calculate_size() const {
  if (data_->width > 0 && data_->height > 0)
    return;
}

int Texture::get_width() const {
  calculate_size();
  return data_->width;
}

int Texture::get_height() const {
  calculate_size();
  return data_->height;
}

fs::path Texture::get_filename() const {
  return data_->filename;
}

//-----------------------------------------------------------------------------

struct FramebufferData {
  GLuint fbo_id;
  std::shared_ptr<Texture> color_texture;
  std::shared_ptr<Texture> depth_texture;
  bool initialized;

  FramebufferData() : initialized(false) {
    FW_CHECKED(glGenFramebuffers(1, &fbo_id));
  }

  FramebufferData(const FramebufferData&) = delete;
  FramebufferData& operator=(const FramebufferData&) = delete;

  ~FramebufferData() {
    FW_CHECKED(glDeleteFramebuffers(1, &fbo_id));
  }

  void ensure_initialized() {
    if (initialized) {
      return;
    }
    initialized = true;

    FW_CHECKED(glBindFramebuffer(GL_FRAMEBUFFER, fbo_id));
    if (depth_texture) {
      GLuint texture_id = depth_texture->get_data()->texture_id;
      FW_CHECKED(glBindTexture(GL_TEXTURE_2D, texture_id));
      FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
      FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
      FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
      FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
      FW_CHECKED(glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture_id, 0));
    }

    if (color_texture) {
      GLuint texture_id = color_texture->get_data()->texture_id;
      FW_CHECKED(glBindTexture(GL_TEXTURE_2D, texture_id));
      FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
      FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
      FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
      FW_CHECKED(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
      FW_CHECKED(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_id, 0));
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      fw::debug << "Framebuffer is not complete, expect errors: " << status << std::endl;
    }
  }
};


//-----------------------------------------------------------------------------
Framebuffer::Framebuffer() : data_(new FramebufferData()) {
}

Framebuffer::~Framebuffer() {
}

void Framebuffer::set_color_buffer(std::shared_ptr<Texture> color_texture) {
  data_->color_texture = color_texture;
}

void Framebuffer::set_depth_buffer(std::shared_ptr<Texture> depth_texture) {
  data_->depth_texture = depth_texture;
}

std::shared_ptr<Texture> Framebuffer::get_color_buffer() const {
  return data_->color_texture;
}

std::shared_ptr<Texture> Framebuffer::get_depth_buffer() const {
  return data_->depth_texture;
}

void Framebuffer::bind() {
  data_->ensure_initialized();
  FW_CHECKED(glBindFramebuffer(GL_FRAMEBUFFER, data_->fbo_id));
}

void Framebuffer::clear() {
  int bits = 0;
  if (data_->color_texture) {
    bits |= GL_COLOR_BUFFER_BIT;
  }
  if (data_->depth_texture) {
    bits |= GL_DEPTH_BUFFER_BIT;
  }
  FW_CHECKED(glClear(bits));
}

void Framebuffer::unbind() {
  FW_CHECKED(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

int Framebuffer::get_width() const {
  if (data_->color_texture) {
    return data_->color_texture->get_width();
  } else if (data_->depth_texture) {
    return data_->depth_texture->get_width();
  } else {
    return 0;
  }
}

int Framebuffer::get_height() const {
  if (data_->color_texture) {
    return data_->color_texture->get_height();
  } else if (data_->depth_texture) {
    return data_->depth_texture->get_height();
  } else {
    return 0;
  }
}

}

#pragma once

#include <memory>
#include <boost/filesystem.hpp>

namespace fw {
class Bitmap;
struct TextureData;
struct FramebufferData;

class Texture {
private:
  std::shared_ptr<TextureData> data_;

  void calculate_size() const;

public:
  Texture();
  ~Texture();

  void create(boost::filesystem::path const &filename);
  void create(std::shared_ptr<fw::Bitmap> bmp);
  void create(fw::Bitmap const &bmp);
  void create(int width, int height, bool is_shadowmap = false);

  // save the contents of this texture to a .png file with the given name
  void save_png(boost::filesystem::path const &filename);

  int get_width() const;
  int get_height() const;

  bool is_created() const {
    return !!data_;
  }

  void bind() const;

  // gets the name of the file we were created from (or an empty string if we
  // weren't created from a file)
  boost::filesystem::path get_filename() const;

  /** Gets the texture_data for this texture (used by framebuffer) */
  std::shared_ptr<TextureData> get_data() const {
    return data_;
  }
};

/** A framebuffer holds either a colour texture, a depth texture or bother, and lets use render to those. */
class Framebuffer {
private:
  std::shared_ptr<FramebufferData> data_;

public:
  Framebuffer();
  ~Framebuffer();

  void set_colour_buffer(std::shared_ptr<Texture> colour_texture);
  void set_depth_buffer(std::shared_ptr<Texture> depth_texture);

  std::shared_ptr<Texture> get_colour_buffer() const;
  std::shared_ptr<Texture> get_depth_buffer() const;

  void bind();
  void clear();
  void unbind();

  int get_width() const;
  int get_height() const;
};

}

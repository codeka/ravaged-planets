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

  // If this is set, we'll call it (once) on the render thread to finish creating the actual TextureData. We do expect
  // to have data_->width, height etc set before the render thread runs though.
  std::function<void(TextureData& data)> data_creator_;

  void calculate_size() const;

public:
  Texture();
  ~Texture();

  void create(boost::filesystem::path const &filename);
  void create(std::shared_ptr<fw::Bitmap> bmp);
  void create(fw::Bitmap const &bmp);
  void create(int width, int height, bool is_shadowmap = false);

  // Ensures we are created before you call bind. Must be called on the render thread.
  void ensure_created();

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
  std::shared_ptr<TextureData> get_data() {
    ensure_created();
    return data_;
  }
};

/** A framebuffer holds either a color texture, a depth texture or bother, and lets use render to those. */
class Framebuffer {
private:
  std::shared_ptr<FramebufferData> data_;

public:
  Framebuffer();
  ~Framebuffer();

  void set_color_buffer(std::shared_ptr<Texture> color_texture);
  void set_depth_buffer(std::shared_ptr<Texture> depth_texture);

  std::shared_ptr<Texture> get_color_buffer() const;
  std::shared_ptr<Texture> get_depth_buffer() const;

  void bind();
  void clear();
  void unbind();

  int get_width() const;
  int get_height() const;
};

}

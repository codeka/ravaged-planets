#pragma once

#include <memory>
#include <boost/filesystem.hpp>

namespace fw {
class Bitmap;
struct TextureData;
struct FramebufferData;

class TextureBase {
public:
  TextureBase() = default;
  virtual ~TextureBase() = default;

  virtual void ensure_created() = 0;
  virtual int get_width() const = 0;
  virtual int get_height() const = 0;
  virtual void bind() const = 0;
};

class Texture : public TextureBase {
private:
  std::shared_ptr<TextureData> data_;

  // If this is set, we'll call it (once) on the render thread to finish creating the actual TextureData. We do expect
  // to have data_->width, height etc set before the render thread runs though.
  std::function<void(TextureData& data)> data_creator_;

  void calculate_size() const;

public:
  Texture();
  virtual ~Texture();

  void create(boost::filesystem::path const &filename);
  void create(std::shared_ptr<fw::Bitmap> bmp);
  void create(fw::Bitmap const &bmp);
  void create(int width, int height, bool is_shadowmap = false);

  // Ensures we are created before you call bind. Must be called on the render thread.
  void ensure_created() override;

  // save the contents of this texture to a .png file with the given name
  void save_png(boost::filesystem::path const &filename);

  int get_width() const override;
  int get_height() const override;

  bool is_created() const {
    return !!data_;
  }

  void bind() const override;

  // gets the name of the file we were created from (or an empty string if we
  // weren't created from a file)
  boost::filesystem::path get_filename() const;

  /** Gets the texture_data for this texture (used by framebuffer) */
  std::shared_ptr<TextureData> get_data() {
    ensure_created();
    return data_;
  }
};

// TextureArray represents an array of textures (surprise!) and allows to bind them all to a single sampler in the
// shader, allowing for more efficient access.
//
// When you create a TextureArray, you must specify the width and height. Any images you add to the texture will be
// automatically resized (all textures in an array must have the same size).
class TextureArray : public TextureBase {
private:
  int width_;
  int height_;
  std::shared_ptr<TextureData> data_;

  // The bitmaps we use to create the texture data. These last only long enough to create the TextureData. Once we have
  // the TextureData, they are released.
  std::vector<std::shared_ptr<fw::Bitmap>> bitmaps_;

public:
  TextureArray(int width, int height);
  virtual ~TextureArray();

  // You cannot call any add() method once the texture has been bound once.
  void add(boost::filesystem::path const& filename);
  void add(std::shared_ptr<fw::Bitmap> bmp);

  // Call on the render thread to ensure we've been created.
  void ensure_created() override;

  int get_width() const override { return width_; }
  int get_height() const override { return height_; }

  void bind() const override;
};

// A framebuffer holds either a color texture, a depth texture or bother, and lets use render to those.
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

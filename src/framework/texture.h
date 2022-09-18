#pragma once

#include <memory>
#include <boost/filesystem.hpp>

namespace fw {
class bitmap;
struct texture_data;
struct framebuffer_data;

class texture {
private:
  std::shared_ptr<texture_data> _data;

  void calculate_size() const;

public:
  texture();
  ~texture();

  void create(boost::filesystem::path const &filename);
  void create(std::shared_ptr<fw::bitmap> bmp);
  void create(fw::bitmap const &bmp);
  void create(int width, int height, bool is_shadowmap = false);

  // save the contents of this texture to a .png file with the given name
  void save_png(boost::filesystem::path const &filename);

  int get_width() const;
  int get_height() const;

  bool is_created() const {
    return (!!_data);
  }

  void bind() const;

  // gets the name of the file we were created from (or an empty string if we
  // weren't created from a file)
  boost::filesystem::path get_filename() const;

  /** Gets the texture_data for this texture (used by framebuffer) */
  std::shared_ptr<texture_data> get_data() const {
    return _data;
  }
};

/** A framebuffer holds either a colour texture, a depth texture or bother, and lets use render to those. */
class framebuffer {
private:
  std::shared_ptr<framebuffer_data> _data;

public:
  framebuffer();
  ~framebuffer();

  void set_colour_buffer(std::shared_ptr<texture> colour_texture);
  void set_depth_buffer(std::shared_ptr<texture> depth_texture);

  std::shared_ptr<texture> get_colour_buffer() const;
  std::shared_ptr<texture> get_depth_buffer() const;

  void bind();
  void clear();
  void unbind();

  int get_width() const;
  int get_height() const;
};

}

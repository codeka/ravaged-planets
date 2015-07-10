#pragma once

#include "colour.h"

namespace fw {

struct bitmap_data;
class texture;
class bitmap;

// Represents a bitmap, always RGBA.
class bitmap {
private:
  bitmap_data *_data;

  // This is called in the destructor or when we're about to assign another bitmap to us, etc.
  void release();

  // Prepares this bitmap for writing
  void prepare_write(int width, int height);

  // Populates our bitmap_data with data from the given file
  void load_bitmap(boost::filesystem::path const &filename);

  // Populates our bitmap_data with data from the given in-memory file
  void load_bitmap(uint8_t const *data, size_t data_size);

  // Populates our bitmap data from the given texture.
  void load_bitmap(texture const &tex);

public:
  // Constructs a blank bitmap.
  bitmap();

  // Constructs a new bitmap that is the given width/height.
  bitmap(int width, int height, uint32_t *argb = 0);

  // Constructs a new bitmap and loads data from the given file.
  bitmap(boost::filesystem::path const &filename);

  // Constructs a new bitmap from data in memory (e.g. a PNG file loaded into memory)
  bitmap(uint8_t const *data, size_t data_size);

  // Constructs a new bitmap from the given texture (if the texture is mip-mapped, we'll use the largest mip-map level)
  bitmap(texture const &tex);

  // Constructs a new bitmap that is a copy of the given bitmap. We won't actually copy the data until you write to it.
  bitmap(bitmap const &copy);

  // Destroys the bitmap and frees all associated data.
  ~bitmap();

  // Assigns this bitmap to what's in the the other one
  bitmap &operator =(fw::bitmap const &copy);

  // Saves the bitmap to the given file.
  void save_bitmap(boost::filesystem::path const &filename) const;

  // Gets the width/height (in pixels) of this image
  int get_width() const;
  int get_height() const;

  // Gets or sets the actual pixel data. We assume the format of the pixels is RGBA and that the given data is big
  // enough to fix this bitmap
  std::vector<uint32_t> const &get_pixels() const;
  void get_pixels(std::vector<uint32_t> &rgba) const;
  void set_pixels(std::vector<uint32_t> const &rgba);

  // Helper methods to get/set the colour of a single pixel.
  fw::colour get_pixel(int x, int y);
  void set_pixel(int x, int y, fw::colour colour);
  void set_pixel(int x, int y, uint32_t rgba);

  // Copies the bitmap data from the given source image to our buffer.
  void copy(fw::bitmap const &src);

  // Resizes the bitmap to the new width/height
  void resize(int new_width, int new_height);
};

}

#pragma once

#include "colour.h"

namespace fw {

struct bitmap_data;
class texture;
class bitmap;

// Blit the given bitmap to the given texture
void blit(bitmap const &src, texture &dest);

// Represents a bitmap
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

  // Gets or sets the actual pixel data. We assume the format of the pixels is ARGB and that the given data is big
  // enough to fix this bitmap
  std::vector<uint32_t> const &get_pixels() const;
  void get_pixels(std::vector<uint32_t> &argb) const;
  void set_pixels(std::vector<uint32_t> const &argb);

  // Helper method to get the colour of a single pixel (this is MUCH slower than using get_pixels() and doing it all
  // at once...)
  fw::colour get_pixel(int x, int y);

  // Copies the bitmap data from the given source image to our buffer.
  void copy(fw::bitmap const &src);

  // Resizes the bitmap to the new width/height, quality is a value from 0 (lowest) to 2 (highest) and determines
  // what "quality level" we use when resizing the image
  void resize(int new_width, int new_height, int quality);
};

}
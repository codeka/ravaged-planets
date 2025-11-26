#pragma once

#include <filesystem>

#include <framework/color.h>

namespace fw {
class Texture;
struct BitmapData;

// Represents a bitmap, always RGBA.
class Bitmap {
private:
  BitmapData *data_;

  // This is called in the destructor or when we're about to assign another bitmap to us, etc.
  void release();

  // Prepares this bitmap for writing
  void prepare_write(int width, int height);

  // Populates our bitmap_data with data from the given file
  void load_bitmap(std::filesystem::path const &filename);

  // Populates our bitmap_data with data from the given in-memory file
  void load_bitmap(uint8_t const *data, size_t data_size);

  // Populates our bitmap data from the given texture.
  void load_bitmap(Texture &tex);

public:
  // Constructs a blank bitmap.
  Bitmap();

  // Constructs a new bitmap that is the given width/height.
  Bitmap(int width, int height, uint32_t *argb = 0);

  // Constructs a new bitmap and loads data from the given file.
  Bitmap(std::filesystem::path const &filename);

  // Constructs a new bitmap from data in memory (e.g. a PNG file loaded into memory)
  Bitmap(uint8_t const *data, size_t data_size);

  // Constructs a new bitmap from the given texture (if the texture is mip-mapped, we'll use the largest mip-map level)
  Bitmap(Texture &tex);

  // Constructs a new bitmap that is a copy of the given bitmap. We won't actually copy the data until you write to it.
  Bitmap(Bitmap const &copy);

  // Destroys the bitmap and frees all associated data.
  ~Bitmap();

  // Assigns this bitmap to what's in the the other one
  Bitmap&operator =(fw::Bitmap const &copy);

  // Gets the filename of this bitmap (if it was loaded from a file).
  std::filesystem::path get_filename() const;

  // Saves the bitmap to the given file.
  void save_bitmap(std::filesystem::path const &filename) const;

  // Gets the width/height (in pixels) of this image
  int get_width() const;
  int get_height() const;

  // Gets or sets the actual pixel data. We assume the format of the pixels is RGBA and that the given data is big
  // enough to fix this bitmap
  std::vector<uint32_t> const &get_pixels() const;
  void get_pixels(std::vector<uint32_t> &rgba) const;
  void set_pixels(std::vector<uint32_t> const &rgba);

  // Helper methods to get/set the color of a single pixel.
  fw::Color get_pixel(int x, int y);
  void set_pixel(int x, int y, fw::Color color);
  void set_pixel(int x, int y, uint32_t rgba);

  // Copies the bitmap data from the given source image to our buffer.
  void copy(fw::Bitmap const &src);

  // Resizes the bitmap to the new width/height
  void resize(int new_width, int new_height);

  // Calculate the "dominant" color of this bitmap.
  fw::Color get_dominant_color() const;
};

}

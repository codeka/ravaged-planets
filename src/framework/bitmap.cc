
#include <framework/framework.h>
#include <framework/bitmap.h>
#include <framework/misc.h>
#include <framework/colour.h>
#include <framework/logging.h>
#include <framework/graphics.h>
#include <framework/exception.h>
#include <framework/texture.h>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <stb/stb_image_resize.h>

namespace fs = boost::filesystem;

namespace fw {

//-------------------------------------------------------------------------
// This class contains the actual bitmap data, which is an array of 32-bit ARGB pixels
struct BitmapData {
  int width;
  int height;
  std::vector<uint32_t> rgba;
  fs::path filename;

  // This is the number of bitmaps that have a reference to us.
  int ref_count;

  // constructs a new BitmapData (just set everything to 0)
  inline BitmapData() {
    width = height = 0;
    ref_count = 1;
  }

  BitmapData(const BitmapData&) = delete;
  BitmapData& operator=(const BitmapData&) = delete;
};

//-------------------------------------------------------------------------
Bitmap::Bitmap() :
    data_(nullptr) {
}

Bitmap::Bitmap(int width, int height, uint32_t *argb /*= 0*/) :
    data_(nullptr) {
  prepare_write(width, height);

  if (argb != nullptr) {
    memcpy(&data_->rgba[0], argb, width * height * sizeof(uint32_t));
  }
}

Bitmap::Bitmap(fs::path const &filename) :
    data_(nullptr) {
  load_bitmap(filename);
}

Bitmap::Bitmap(uint8_t const *data, size_t data_size) :
    data_(nullptr) {
  load_bitmap(data, data_size);
}

Bitmap::Bitmap(Texture const &tex) :
    data_(nullptr) {
  load_bitmap(tex);
}

Bitmap::Bitmap(Bitmap const &copy) {
  data_ = copy.data_;
  data_->ref_count++;
}

Bitmap::~Bitmap() {
  release();
}

Bitmap& Bitmap::operator =(fw::Bitmap const &copy) {
  // ignore self assignment
  if (this == &copy)
    return (*this);

  release();

  data_ = copy.data_;
  data_->ref_count++;

  return (*this);
}

void Bitmap::release() {
  if (data_ != 0) {
    data_->ref_count--;
    if (data_->ref_count == 0) {
      delete data_;
    }
  }
}

void Bitmap::prepare_write(int width, int height) {
  if (data_ == nullptr || data_->ref_count > 1) {
    // we'll have to create a new bitmapdata_ if there's currently no data or
    // the ref_count is > 1
    if (data_ != nullptr)
      data_->ref_count--;

    data_ = new BitmapData();
  }

  // make sure the pixel buffer is big enough to hold the required width/height
  data_->rgba.resize(width * height);
  data_->width = width;
  data_->height = height;
  data_->filename = fs::path();
}

void Bitmap::load_bitmap(fs::path const &filename) {
  debug << boost::format("loading image: %1%") % filename << std::endl;
  prepare_write(0, 0);
  data_->filename = filename;

  int channels;
  unsigned char *pixels = stbi_load(filename.string().c_str(), &data_->width, &data_->height, &channels, 4);
  if (pixels == nullptr) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Error reading image."));
  }

  // copy pixels from what stb returned into our own buffer
  data_->rgba.resize(data_->width * data_->height);
  memcpy(data_->rgba.data(), reinterpret_cast<uint32_t const *>(pixels),
      data_->width * data_->height * sizeof(uint32_t));

  // don't need this anymore
  stbi_image_free(pixels);
}

// Populates our bitmapdata_ with data from the given in-memory file
void Bitmap::load_bitmap(uint8_t const *data, size_t data_size) {
  int channels;
  unsigned char *pixels = stbi_load_from_memory(
      reinterpret_cast<unsigned char const *>(data), data_size, &data_->width, &data_->height, &channels, 4);
  if (pixels == nullptr) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Error reading image."));
  }

  // copy pixels from what stb returned into our own buffer
  memcpy(data_->rgba.data(), reinterpret_cast<uint32_t const *>(pixels), data_->width * data_->height);

  // don't need this anymore
  stbi_image_free(pixels);
}

void Bitmap::load_bitmap(Texture const &tex) {
  FW_ENSURE_RENDER_THREAD();

  prepare_write(tex.get_width(), tex.get_height());
  tex.bind();
  FW_CHECKED(glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data_->rgba.data()));

  // OpenGL returns images with 0 at the bottom, but we want 0 at the top so we have to flip it
  uint32_t *row_buffer = new uint32_t[data_->width];
  for (int y = 0; y < data_->height / 2; y++) {
    memcpy(row_buffer, &data_->rgba[y * data_->width], sizeof(uint32_t) * data_->width);
    memcpy(&data_->rgba[y * data_->width], &data_->rgba[(data_->height - y - 1) * data_->width], sizeof(uint32_t) * data_->width);
    memcpy(&data_->rgba[(data_->height - y - 1) * data_->width], row_buffer, sizeof(uint32_t) * data_->width);
  }
  delete[] row_buffer;
}

void Bitmap::save_bitmap(fs::path const &filename) const {
  if (data_ == 0)
    return;

  debug << boost::format("saving image: %1%") % filename << std::endl;

  fs::path path(filename);
  if (fs::exists(path)) {
    fs::remove(path);
  }

  int res;
  if (filename.extension() == ".png") {
    res = stbi_write_png(filename.string().c_str(), data_->width, data_->height, 4,
        reinterpret_cast<void const *>(data_->rgba.data()), 0);
  } else if (filename.extension() == ".bmp") {
    res = stbi_write_bmp(filename.string().c_str(), data_->width, data_->height, 4,
        reinterpret_cast<void const *>(data_->rgba.data()));
  } else {
    res = 0;
  }
  if (res == 0) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Error writing file."));
  }
}

int Bitmap::get_width() const {
  if (data_ == 0)
    return 0;

  return data_->width;
}

int Bitmap::get_height() const {
  if (data_ == 0)
    return 0;

  return data_->height;
}

fs::path Bitmap::get_filename() const {
  if (data_ == nullptr) {
    return fs::path();
  }
  return data_->filename;
}

std::vector<uint32_t> const & Bitmap::get_pixels() const {
  return data_->rgba;
}

void Bitmap::get_pixels(std::vector<uint32_t> &rgba) const {
  if (data_ == 0)
    return;

  int width = get_width();
  int height = get_height();

  // make sure it's big enough to hold the data
  rgba.resize(width * height);
  memcpy(rgba.data(), data_->rgba.data(), width * height * sizeof(uint32_t));
}

void Bitmap::set_pixels(std::vector<uint32_t> const &rgba) {
  int width = get_width();
  int height = get_height();

  prepare_write(width, height);
  memcpy(data_->rgba.data(), rgba.data(), width * height * sizeof(uint32_t));
}

fw::colour Bitmap::get_pixel(int x, int y) {
  return fw::colour::from_rgba(data_->rgba[(get_width() * y) + x]);
}

void Bitmap::set_pixel(int x, int y, fw::colour colour) {
  data_->rgba[(get_width() * y) + x] = colour.to_rgba();
}

void Bitmap::set_pixel(int x, int y, uint32_t rgba) {
  data_->rgba[(get_width() * y) + x] = rgba;
}

void Bitmap::resize(int new_width, int new_height) {
  int curr_width = get_width();
  int curr_height = get_height();

  if (curr_width == new_width && curr_height == new_height) {
    return;
  }

  std::vector<uint32_t> resized(new_width * new_height);
  stbir_resize_uint8(reinterpret_cast<unsigned char const *>(data_->rgba.data()), curr_width, curr_height, 0,
      reinterpret_cast<unsigned char *>(resized.data()), new_width, new_height, 0, 4);

  prepare_write(new_width, new_height);
  set_pixels(resized);
}

/**
 * Gets the "dominant" colour of this bitmap. For now, we simple return the average colour, but something like finding
 * the most common colour or something might be better.
 */
fw::colour Bitmap::get_dominant_colour() const {
  fw::colour average(0, 0, 0, 0);
  for(uint32_t rgba : data_->rgba) {
    average += fw::colour::from_abgr(rgba);
  }

  average /= static_cast<double>(data_->rgba.size());
  return average.clamp();
}

}

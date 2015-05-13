#include <framework/bitmap.h>
#include <framework/misc.h>
#include <framework/colour.h>
#include <framework/logging.h>
#include <framework/graphics.h>
#include <framework/exception.h>
#include <framework/texture.h>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace fs = boost::filesystem;

namespace fw {
//-------------------------------------------------------------------------
// This class contains the actual bitmap data, which is an array of 32-bit ARGB pixels
struct bitmap_data: private boost::noncopyable {
  int width;
  int height;
  std::vector<uint32_t> rgba;

  // This is the number of bitmaps that have a reference to us.
  int ref_count;

  // constructs a new bitmap_data (just set everything to 0)
  inline bitmap_data() {
    width = height = 0;
    ref_count = 1;
  }
};

//-------------------------------------------------------------------------
void blit(fw::bitmap const &src, fw::texture &dest) {
  /*
   int width = src.get_width();
   int height = src.get_height();

   if (dest.get_width() != width || dest.get_height() != height)
   {
   // we want the texture to be exactly the same width/height as us, so
   // if it's not, resize it (log a warning though, cause we should try
   // to avoid this as much as possible - the caller can resize the texture
   // if they really need to)
   debug << boost::format("warning: resizing texture to fix bitmap size: (%1%, %2%), original texture size was: (%3%, %4%)")
   % width % height % dest.get_width() % dest.get_height() << std::endl;
   dest.create(0, width, height);
   }

   // grab the image data from the bitmap in a nice, easy to use format (RGBA, 8 bits per pixel)
   std::vector<uint32_t> const &buffer = src.get_pixels();
   copy_pixels(dest.get_d3dtexture(), &buffer[0], width, height);
   */
}

//-------------------------------------------------------------------------
bitmap::bitmap() :
    _data(0) {
}

bitmap::bitmap(int width, int height, uint32_t *argb /*= 0*/) :
    _data(0) {
  prepare_write(width, height);

  if (argb != 0) {
    memcpy(&_data->rgba[0], argb, width * height * sizeof(uint32_t));
  }
}

bitmap::bitmap(fs::path const &filename) :
    _data(0) {
  load_bitmap(filename);
}

bitmap::bitmap(uint8_t const *data, size_t data_size) :
    _data(0) {
  load_bitmap(data, data_size);
}

bitmap::bitmap(texture const &tex) :
    _data(0) {
//		IDirect3DTexture9 *texture = tex.get_d3dtexture();
//		load_bitmap(texture);
}

bitmap::bitmap(bitmap const &copy) {
  _data = copy._data;
  _data->ref_count++;
}

bitmap::~bitmap() {
  release();
}

bitmap &bitmap::operator =(fw::bitmap const &copy) {
  // ignore self assignment
  if (this == &copy)
    return (*this);

  release();

  _data = copy._data;
  _data->ref_count++;

  return (*this);
}

void bitmap::release() {
  if (_data != 0) {
    _data->ref_count--;
    if (_data->ref_count == 0) {
      delete _data;
    }
  }
}

void bitmap::prepare_write(int width, int height) {
  if (_data == 0 || _data->ref_count > 1) {
    // we'll have to create a new bitmap_data if there's currently no data or
    // the ref_count is > 1
    if (_data != 0)
      _data->ref_count--;

    _data = new bitmap_data();
  }

  // make sure the pixel buffer is big enough to hold the required width/height
  _data->rgba.resize(width * height);
  _data->width = width;
  _data->height = height;
}

void bitmap::load_bitmap(fs::path const &filename) {
  debug << boost::format("loading image: %1%") % filename << std::endl;
  prepare_write(0, 0);

  int channels;
  unsigned char *pixels = stbi_load(filename.string().c_str(), &_data->width, &_data->height, &channels, 4);
  if (pixels == nullptr) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Error reading image."));
  }

  // copy pixels from what stb returned into our own buffer
  memcpy(_data->rgba.data(), reinterpret_cast<uint32_t const *>(pixels), _data->width * _data->height);

  // don't need this anymore
  stbi_image_free(pixels);
}

// Populates our bitmap_data with data from the given in-memory file
void bitmap::load_bitmap(uint8_t const *data, size_t data_size) {
  int channels;
  unsigned char *pixels = stbi_load_from_memory(
      reinterpret_cast<unsigned char const *>(data), data_size, &_data->width, &_data->height, &channels, 4);
  if (pixels == nullptr) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Error reading image."));
  }

  // copy pixels from what stb returned into our own buffer
  memcpy(_data->rgba.data(), reinterpret_cast<uint32_t const *>(pixels), _data->width * _data->height);

  // don't need this anymore
  stbi_image_free(pixels);
}

void bitmap::save_bitmap(fs::path const &filename) const {
  if (_data == 0)
    return;

  debug << boost::format("saving image: %1%") % filename << std::endl;

  fs::path path(filename);
  if (fs::exists(path)) {
    fs::remove(path);
  }

  int res = stbi_write_bmp(filename.c_str(), _data->width, _data->height, 4,
      reinterpret_cast<unsigned char *>(_data->rgba.data()));
  if (res == 0) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Error writing file."));
  }
}

int bitmap::get_width() const {
  if (_data == 0)
    return 0;

  return _data->width;
}

int bitmap::get_height() const {
  if (_data == 0)
    return 0;

  return _data->height;
}

std::vector<uint32_t> const &bitmap::get_pixels() const {
  return _data->rgba;
}

void bitmap::get_pixels(std::vector<uint32_t> &rgba) const {
  if (_data == 0)
    return;

  int width = get_width();
  int height = get_height();

  // make sure it's big enough to hold the data
  rgba.resize(width * height);
  memcpy(rgba.data(), _data->rgba.data(), width * height * sizeof(uint32_t));
}

void bitmap::set_pixels(std::vector<uint32_t> const &rgba) {
  int width = get_width();
  int height = get_height();

  prepare_write(width, height);
  memcpy(_data->rgba.data(), rgba.data(), width * height * sizeof(uint32_t));
}

fw::colour bitmap::get_pixel(int x, int y) {
  return fw::colour::from_rgba(_data->rgba[(get_width() * y) + x]);
}

void bitmap::set_pixel(int x, int y, fw::colour colour) {
  _data->rgba[(get_width() * y) + x] = colour.to_rgba();
}

void bitmap::resize(int new_width, int new_height, int quality) {
  int curr_width = get_width();
  int curr_height = get_height();

  if (curr_width == new_width && curr_height == new_height)
    return;

  if (new_width > curr_width || new_height > curr_height) {
    BOOST_THROW_EXCEPTION(fw::exception()<< fw::message_error_info("up-scaling is not supported"));
  }

  int scale_x = curr_width / new_width;
  if (scale_x < 1)
    scale_x = 1;

  int scale_y = curr_height / new_height;
  if (scale_y < 1)
    scale_y = 1;

  std::vector<uint32_t> resized(new_width * new_height);
  for (int y = 0; y < new_height; y++) {
    for (int x = 0; x < new_width; x++) {
      // we get the average colour of the pixels in the rectangle
      // that the current pixel occupies

      fw::colour average(0, 0, 0, 0);
      int n = 0;
      for (int y1 = (y * scale_y); y1 < (y + 1) * scale_y; y1++) {
        if (y1 >= curr_height)
          break;

        for (int x1 = (x * scale_x); x1 < (x + 1) * scale_x; x1++) {
          if (x1 >= curr_width)
            break;

          average += fw::colour::from_rgba(_data->rgba[(y1 * curr_width) + x1]);
          n++;
        }
      }

      average /= static_cast<double>(n);
      average = average.clamp();
      resized[(y * new_width) + x] = average.to_argb();
    }
  }

  prepare_write(new_width, new_height);
  set_pixels(resized);
}

}

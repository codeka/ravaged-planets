#pragma once

#include <memory>
#include <boost/filesystem.hpp>

namespace fw {

struct texture_data;

class texture {
private:
  std::shared_ptr<texture_data> _data;

  void calculate_size() const;

public:
  texture();
  ~texture();

  void create(boost::filesystem::path const &filename);
  void create(int width, int height, bool dynamic = false);
//  void create(int width, int height, bool dynamic = true,
//      D3DFORMAT format = D3DFMT_A8R8G8B8, int mipmap_levels = 1, D3DPOOL pool =
//          D3DPOOL_MANAGED);
//  void create_rendertarget(int width, int height, D3DFORMAT format =
//      D3DFMT_A8R8G8B8, int usage = 0, D3DPOOL pool = D3DPOOL_DEFAULT,
//      int mipmap_levels = 1);
//  void create_from_memory(void *buffer, int buffer_size, int *width = 0,
//      int *height = 0, int mipmaps_levels = 0,
//      D3DFORMAT format = D3DFMT_UNKNOWN, D3DPOOL pool = D3DPOOL_MANAGED);

  // save the contents of this texture to a .png file with the given name
  void save_png(boost::filesystem::path const &filename);

  int get_width() const;
  int get_height() const;

  bool is_created() const {
    return (!!_data);
  }

  void bind();

  // gets the name of the file we were created from (or an empty string if we
  // weren't created from a file)
  boost::filesystem::path get_filename() const;
};

}

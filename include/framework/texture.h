#pragma once

#include <memory>
#include <boost/filesystem.hpp>

namespace fw {
class bitmap;
struct texture_data;

class texture {
private:
  std::shared_ptr<texture_data> _data;

  void calculate_size() const;

public:
  texture();
  ~texture();

  void create(boost::filesystem::path const &filename);
  void create(std::shared_ptr<fw::bitmap> bmp, bool dynamic = false);
  void create(fw::bitmap const &bmp, bool dynamic = false);

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
};

}

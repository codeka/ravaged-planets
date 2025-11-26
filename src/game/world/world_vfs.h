#pragma once

#include <filesystem>
#include <fstream>
#include <memory>

#include <framework/bitmap.h>

namespace game {

// A "summary" of a single map file. Things like the name, size and so on, useful for displaying
// in a list.
class WorldSummary {
private:
  std::string name_;
  mutable bool extra_loaded_;
  mutable std::string description_;
  mutable std::string author_;
  mutable int width_;
  mutable int height_;
  mutable std::shared_ptr<fw::Bitmap> screenshot_;
  mutable int num_players_;

  // this is called when you request any of the "extra" info (which is anything other than the name), we load
  // up the data on-demand.
  void ensure_extra_loaded() const;

  // parse the <mapdesc> file and populate our extra stuff.
  void parse_mapdesc_file(std::filesystem::path const &filename) const;

public:
  WorldSummary();
  WorldSummary(WorldSummary const &copy);
  ~WorldSummary();

  // initialize our properties from the given map file (or directory)
  void initialize(std::string map_file);

  std::string get_name() const {
    return name_;
  }
  std::string get_description() const {
    ensure_extra_loaded();
    return description_;
  }
  std::string get_author() const {
    ensure_extra_loaded();
    return author_;
  }
  int get_width() const {
    ensure_extra_loaded();
    return width_;
  }
  int get_height() const {
    ensure_extra_loaded();
    return height_;
  }
  std::shared_ptr<fw::Bitmap> get_screenshot() const {
    ensure_extra_loaded();
    return screenshot_;
  }
  int get_num_players() const {
    ensure_extra_loaded();
    return num_players_;
  }
};

// Represents a named entry in the "world file".
class WorldFileEntry {
private:
  std::string full_path_;
  std::fstream stream_;
  bool for_write_;

  void copy(WorldFileEntry const &copy);
  void ensure_open(bool throw_on_error = true);

public:
  WorldFileEntry(std::string full_path, bool for_write);
  WorldFileEntry(WorldFileEntry const &copy);
  ~WorldFileEntry();

  WorldFileEntry &operator =(WorldFileEntry const &copy);

  // gets the full path to this file
  std::string get_full_path() const {
    return full_path_;
  }

  // writes num_bytes into the file
  void write(void const *buffer, int num_bytes);

  // writes a line of text, encoded as UTF-8, to the file.
  void write(std::string const &line);

  // reads num_bytes from the file into the buffer
  void read(void *buffer, int num_bytes);

  // returns a value that indicates whether the file exists or not
  bool exists();

  // closes the file, if we have it open
  void close();
};

// Represents a "world file", which is a collection of \ref world_file_entry objects.
class WorldFile {
private:
  std::string path_;

public:
  WorldFile(std::string path);

  WorldFileEntry get_entry(std::string name, bool for_write);
};

// This class represents a "virtual file system" containing the world file(s). Basically this just
// combines the world files in the install directory and the user's personal directory.
class WorldVfs {
public:
  WorldVfs();
  ~WorldVfs();

  // gets a list of all the maps
  std::vector<WorldSummary> list_maps();

  // opens a new world_file with the complete details of the given map
  WorldFile open_file(std::string name, bool for_writing = false);
};

}

#include <vector>

#include <boost/algorithm/string.hpp>

#include <framework/logging.h>
#include <framework/paths.h>
#include <framework/exception.h>
#include <framework/bitmap.h>
#include <framework/xml.h>

#include <game/world/world_vfs.h>

namespace fs = boost::filesystem;

// populate the given world summary based on the maps found in the given path
void populate_maps(std::vector<game::world_summary> &list, fs::path path);

// looks for the full path to the map with the given name
fs::path find_map(std::string name);

namespace game {

world_summary::world_summary() :
    _name(""), _extra_loaded(false), _screenshot(nullptr), _width(0), _height(0), _num_players(0) {
}

world_summary::world_summary(world_summary const &copy) :
    _name(copy._name), _extra_loaded(false), _screenshot(nullptr), _width(0), _height(0), _num_players(0) {
}

world_summary::~world_summary() {
}

void world_summary::ensure_extra_loaded() const {
  if (_extra_loaded)
    return;

  fs::path full_path(find_map(_name));

  auto screenshot_path = full_path / "screenshot.png";
  if (fs::exists(screenshot_path)) {
    _screenshot = std::shared_ptr<fw::bitmap>(new fw::bitmap(full_path / "screenshot.png"));
  }

  parse_mapdesc_file(full_path / (_name + ".mapdesc"));

  _extra_loaded = true;
}

void world_summary::parse_mapdesc_file(fs::path const &filename) const {
  fw::xml_element xml = fw::load_xml(filename, "mapdesc", 1);

  for (fw::xml_element child = xml.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
    if (child.get_value() == "description") {
      _description = child.get_text();
    } else if (child.get_value() == "author") {
      _author = child.get_text();
    } else if (child.get_value() == "size") {
      _width = boost::lexical_cast<int>(child.get_attribute("width"));
      _height = boost::lexical_cast<int>(child.get_attribute("height"));
    } else if (child.get_value() == "players") {
      _num_players = 0;
      for (fw::xml_element player = child.get_first_child(); player.is_valid(); player = player.get_next_sibling()) {
        if (player.get_value() == "player") {
          _num_players ++;
        } else {
          fw::debug << boost::format("WARN: unknown child of <players>: %1%") % child.get_value() << std::endl;
        }
      }
    } else {
      fw::debug << boost::format("WARN: unknown child of <mapdesc>: %1%") % child.get_value() << std::endl;
    }
  }
}

void world_summary::initialize(std::string map_file) {
  _name = map_file;
}

//----------------------------------------------------------------------------

world_vfs::world_vfs() {
}

world_vfs::~world_vfs() {
}

std::vector<world_summary> world_vfs::list_maps() {
  std::vector<world_summary> list;
  populate_maps(list, fw::install_base_path() / "maps");
  populate_maps(list, fw::user_base_path() / "maps");

  // todo: sort

  return list;
}

world_file world_vfs::open_file(std::string name, bool for_writing /*= false*/) {
  // check the user's profile directory first
  fs::path map_path = fw::user_base_path() / "maps";
  fs::path full_path = map_path / name;
  if (fs::is_directory(full_path) || for_writing) {
    // if it's for writing, we'll want to create the above directory and write the files
    // there anyway. At least for now, that's the easiest way....
    fs::create_directories(full_path);
    return world_file(full_path.string());
  }

  if (!for_writing) {
    // if it's not for writing, check the install directory as well
    map_path = fw::install_base_path() / "maps";
    fs::path full_path = map_path / name;
    return world_file(full_path.string());
  }

  // todo: not for writing, check other locations...
  BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("map doesn't exist"));
  return world_file(""); // can't get here (the above line throws an exception)
}

//-------------------------------------------------------------------------

world_file_entry::world_file_entry(std::string full_path, bool for_write) :
    _full_path(full_path), _for_write(for_write) {
}

world_file_entry::world_file_entry(world_file_entry const &copy) : _for_write(false) {
  this->copy(copy);
}

world_file_entry::~world_file_entry() {
  close();
}

world_file_entry &world_file_entry::operator =(world_file_entry const &copy) {
  this->copy(copy);
  return (*this);
}

void world_file_entry::copy(world_file_entry const &copy) {
  close();

  _full_path = copy._full_path;
  _stream.copyfmt(copy._stream);
  _for_write = copy._for_write;
}

void world_file_entry::ensure_open(bool throw_on_error) {
  if (_stream.is_open()) {
    return;
  }

  if (_for_write) {
    _stream.open(_full_path.c_str(), std::ios::out);
  } else {
    _stream.open(_full_path.c_str(), std::ios::in);
  }
  if (_stream.fail()) {
    if (throw_on_error) {
      BOOST_THROW_EXCEPTION(fw::exception() << fw::filename_error_info(_full_path));
    }
  }
}

void world_file_entry::close() {
  if (_stream.is_open()) {
    _stream.close();
  }
}

void world_file_entry::write(void const *buffer, int num_bytes) {
  ensure_open();
  _stream.write(reinterpret_cast<char const *>(buffer), num_bytes);
}

void world_file_entry::write(std::string const &line) {
  std::string full_line = boost::algorithm::trim_right_copy(line);
  full_line += "\r\n";

  write(full_line.c_str(), full_line.length());
}

void world_file_entry::read(void *buffer, int num_bytes) {
  ensure_open();
  _stream.read(reinterpret_cast<char *>(buffer), num_bytes);
}

bool world_file_entry::exists() {
  ensure_open(false);
  return (_stream.is_open());
}

//-------------------------------------------------------------------------

world_file::world_file(std::string path) :
    _path(path) {
}

world_file_entry world_file::get_entry(std::string name, bool for_write) {
  fs::path file_path = fs::path(_path) / name;
  return world_file_entry(file_path.string(), for_write);
}
}

void populate_maps(std::vector<game::world_summary> &list, fs::path path) {
  fw::debug << boost::format("populating maps from: %1%") % path.string()
      << std::endl;

  if (!fs::exists(path) || !fs::is_directory(path))
    return;

  for (fs::directory_iterator it(path); it != fs::directory_iterator(); ++it) {
    fs::path p(*it);
    fw::debug << boost::format("  - %1%") % p.string() << std::endl;
    if (fs::is_directory(p)) {
      game::world_summary ws;
      ws.initialize(p.leaf().string());

      // todo: see if it already exists before adding it
      list.push_back(ws);
    }
  }
}

fs::path find_map(std::string name) {
  fs::path p(fw::install_base_path() / "maps" / name);
  if (fs::is_directory(p))
    return p;

  p = fw::user_base_path() / "maps" / name;
  if (fs::is_directory(p))
    return p;

  BOOST_THROW_EXCEPTION(fw::exception()
      << fw::message_error_info("could not find map!")
      << fw::filename_error_info(name));
  return "";
}

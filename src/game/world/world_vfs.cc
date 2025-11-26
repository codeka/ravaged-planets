#include <filesystem>
#include <vector>

#include <boost/algorithm/string.hpp>

#include <framework/logging.h>
#include <framework/paths.h>
#include <framework/exception.h>
#include <framework/bitmap.h>
#include <framework/xml.h>

#include <game/world/world_vfs.h>

namespace fs = std::filesystem;

// populate the given world summary based on the maps found in the given path
void populate_maps(std::vector<game::WorldSummary> &list, fs::path path);

// looks for the full path to the map with the given name
fs::path find_map(std::string name);

namespace game {

WorldSummary::WorldSummary() :
    name_(""), extra_loaded_(false), screenshot_(nullptr), width_(0), height_(0), num_players_(0) {
}

WorldSummary::WorldSummary(WorldSummary const &copy) :
    name_(copy.name_), extra_loaded_(false), screenshot_(nullptr), width_(0), height_(0), num_players_(0) {
}

WorldSummary::~WorldSummary() {
}

void WorldSummary::ensure_extra_loaded() const {
  if (extra_loaded_)
    return;

  fs::path full_path(find_map(name_));

  auto screenshot_path = full_path / "screenshot.png";
  if (fs::exists(screenshot_path)) {
    screenshot_ = std::make_shared<fw::Bitmap>(full_path / "screenshot.png");
  }

  parse_mapdesc_file(full_path / (name_ + ".mapdesc"));

  extra_loaded_ = true;
}

void WorldSummary::parse_mapdesc_file(fs::path const &filename) const {
  fw::XmlElement xml = fw::load_xml(filename, "mapdesc", 1);

  for (fw::XmlElement child = xml.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
    if (child.get_value() == "description") {
      description_ = child.get_text();
    } else if (child.get_value() == "author") {
      author_ = child.get_text();
    } else if (child.get_value() == "size") {
      width_ = boost::lexical_cast<int>(child.get_attribute("width"));
      height_ = boost::lexical_cast<int>(child.get_attribute("height"));
    } else if (child.get_value() == "players") {
      num_players_ = 0;
      for (fw::XmlElement player = child.get_first_child(); player.is_valid(); player = player.get_next_sibling()) {
        if (player.get_value() == "player") {
          num_players_ ++;
        } else {
          fw::debug << boost::format("WARN: unknown child of <players>: %1%") % child.get_value() << std::endl;
        }
      }
    } else {
      fw::debug << boost::format("WARN: unknown child of <mapdesc>: %1%") % child.get_value() << std::endl;
    }
  }
}

void WorldSummary::initialize(std::string map_file) {
  name_ = map_file;
}

//----------------------------------------------------------------------------

WorldVfs::WorldVfs() {
}

WorldVfs::~WorldVfs() {
}

std::vector<WorldSummary> WorldVfs::list_maps() {
  std::vector<WorldSummary> list;
  populate_maps(list, fw::install_base_path() / "maps");
  populate_maps(list, fw::user_base_path() / "maps");

  // todo: sort

  return list;
}

WorldFile WorldVfs::open_file(std::string name, bool for_writing /*= false*/) {
  // check the user's profile directory first
  fs::path map_path = fw::user_base_path() / "maps";
  fs::path full_path = map_path / name;
  if (fs::is_directory(full_path) || for_writing) {
    // if it's for writing, we'll want to create the above directory and write the files
    // there anyway. At least for now, that's the easiest way....
    fs::create_directories(full_path);
    return WorldFile(full_path.string());
  }

  if (!for_writing) {
    // if it's not for writing, check the install directory as well
    map_path = fw::install_base_path() / "maps";
    fs::path full_path = map_path / name;
    return WorldFile(full_path.string());
  }

  // todo: not for writing, check other locations...
  BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("map doesn't exist"));
  return WorldFile(""); // can't get here (the above line throws an exception)
}

//-------------------------------------------------------------------------

WorldFileEntry::WorldFileEntry(std::string full_path, bool for_write) :
    full_path_(full_path), for_write_(for_write) {
}

WorldFileEntry::WorldFileEntry(WorldFileEntry const &copy) : for_write_(false) {
  this->copy(copy);
}

WorldFileEntry::~WorldFileEntry() {
  close();
}

WorldFileEntry &WorldFileEntry::operator =(WorldFileEntry const &copy) {
  this->copy(copy);
  return (*this);
}

void WorldFileEntry::copy(WorldFileEntry const &copy) {
  close();

  full_path_ = copy.full_path_;
  stream_.copyfmt(copy.stream_);
  for_write_ = copy.for_write_;
}

void WorldFileEntry::ensure_open(bool throw_on_error) {
  if (stream_.is_open()) {
    return;
  }

  if (for_write_) {
    stream_.open(full_path_.c_str(), std::ios::out | std::ifstream::binary);
  } else {
    stream_.open(full_path_.c_str(), std::ios::in | std::ifstream::binary);
  }
  if (stream_.fail()) {
    if (throw_on_error) {
      BOOST_THROW_EXCEPTION(fw::Exception() << fw::filename_error_info(full_path_));
    }
  }
}

void WorldFileEntry::close() {
  if (stream_.is_open()) {
    stream_.close();
  }
}

void WorldFileEntry::write(void const *buffer, int num_bytes) {
  ensure_open();
  stream_.write(reinterpret_cast<char const *>(buffer), num_bytes);
}

void WorldFileEntry::write(std::string const &line) {
  std::string full_line = boost::algorithm::trim_right_copy(line);
  full_line += "\r\n";

  write(full_line.c_str(), full_line.length());
}

void WorldFileEntry::read(void *buffer, int num_bytes) {
  ensure_open();
  stream_.read(reinterpret_cast<char *>(buffer), num_bytes);
}

bool WorldFileEntry::exists() {
  ensure_open(false);
  return (stream_.is_open());
}

//-------------------------------------------------------------------------

WorldFile::WorldFile(std::string path) :
    path_(path) {
}

WorldFileEntry WorldFile::get_entry(std::string name, bool for_write) {
  fs::path file_path = fs::path(path_) / name;
  return WorldFileEntry(file_path.string(), for_write);
}
}

void populate_maps(std::vector<game::WorldSummary> &list, fs::path path) {
  fw::debug << boost::format("populating maps from: %1%") % path.string()
      << std::endl;

  if (!fs::exists(path) || !fs::is_directory(path))
    return;

  for (fs::directory_iterator it(path); it != fs::directory_iterator(); ++it) {
    fs::path p(*it);
    fw::debug << boost::format("  - %1%") % p.string() << std::endl;
    if (fs::is_directory(p)) {
      game::WorldSummary ws;
      ws.initialize(p.filename().string());

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

  BOOST_THROW_EXCEPTION(fw::Exception()
      << fw::message_error_info("could not find map!")
      << fw::filename_error_info(name));
  return "";
}

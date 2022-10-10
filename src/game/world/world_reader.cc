#include <memory>

#include <framework/bitmap.h>
#include <framework/framework.h>
#include <framework/logging.h>
#include <framework/graphics.h>
#include <framework/misc.h>
#include <framework/exception.h>
#include <framework/xml.h>

#include <game/world/world_reader.h>
#include <game/world/world.h>
#include <game/world/world_vfs.h>
#include <game/world/terrain.h>

namespace game {

WorldReader::WorldReader() :
    terrain_(nullptr) {
}

WorldReader::~WorldReader() {
}

void WorldReader::read(std::string name) {
  WorldVfs vfs;
  WorldFile wf = vfs.open_file(name, false);

  int version;
  int trn_width;
  int trn_length;

  WorldFileEntry wfe = wf.get_entry("heightfield", false /* for_write */);
  wfe.read(&version, sizeof(int));
  if (version != 1) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("unknown terrain version"));
  }

  wfe.read(&trn_width, sizeof(int));
  wfe.read(&trn_length, sizeof(int));

  float *height_data = new float[trn_width * trn_length];
  wfe.read(height_data, trn_width * trn_length * sizeof(float));

  name_ = name;
  terrain_ = create_terrain(trn_width, trn_length, height_data);

  for (int patch_z = 0; patch_z < terrain_->get_patches_length(); patch_z++) {
    for (int patch_x = 0; patch_x < terrain_->get_patches_width(); patch_x++) {
      std::string name = (boost::format("splatt-%1%-%2%.png") % patch_x % patch_z).str();
      wfe = wf.get_entry(name, false /* for_write */);

      fw::Bitmap splatt(wfe.get_full_path().c_str());
      terrain_->set_splatt(patch_x, patch_z, splatt);
    }
  }

  wfe = wf.get_entry("minimap.png", false /* for_write */);
  if (wfe.exists()) {
    wfe.close();

    minimap_background_ = std::make_shared<fw::Bitmap>(wfe.get_full_path());
  }

  wfe = wf.get_entry("screenshot.png", false /* for_write */);
  if (wfe.exists()) {
    wfe.close();

    screenshot_ = std::make_shared<fw::Bitmap>(wfe.get_full_path());
  }

  wfe = wf.get_entry(name + ".mapdesc", false /* for_write */);
  if (wfe.exists()) {
    wfe.close();

    fw::XmlElement root(fw::load_xml(wfe.get_full_path(), "mapdesc", 1));
    read_mapdesc(root);
  }

  wfe = wf.get_entry("collision_data", false /* for_write */);
  if (wfe.exists()) {
    read_collision_data(wfe);
  }
}

void WorldReader::read_collision_data(WorldFileEntry &wfe) {
  int version;
  wfe.read(&version, sizeof(int));
  if (version != 1) {
    BOOST_THROW_EXCEPTION(fw::Exception()
        << fw::message_error_info("unknown collision_data version"));
  }

  int width, length;
  wfe.read(&width, sizeof(int));
  wfe.read(&length, sizeof(int));

  for (int i = 0; i < (width * length); i++) {
    uint8_t n;
    wfe.read(&n, sizeof(uint8_t));
    terrain_->collision_data_.push_back(n != 0);
  }
}

void WorldReader::read_mapdesc(fw::XmlElement root) {
  for (fw::XmlElement child = root.get_first_child(); child.is_valid(); child =
      child.get_next_sibling()) {
    if (child.get_value() == "description") {
      description_ = child.get_text();
    } else if (child.get_value() == "author") {
      author_ = child.get_text();
    } else if (child.get_value() == "size") {
    } else if (child.get_value() == "players") {
      read_mapdesc_players(child);
    } else {
      BOOST_THROW_EXCEPTION(fw::Exception()
          << fw::message_error_info("Unknown child element of <mapdesc> node!"));
    }
  }
}

void WorldReader::read_mapdesc_players(fw::XmlElement players_node) {
  for (fw::XmlElement child = players_node.get_first_child(); child.is_valid();
      child = child.get_next_sibling()) {
    if (child.get_value() != "player") {
      BOOST_THROW_EXCEPTION(fw::Exception()
          << fw::message_error_info("Unknown child element of <players> node!"));
    }

    int player_no = boost::lexical_cast<int>(child.get_attribute("no"));
    std::vector<float> start = fw::split<float>(child.get_attribute("start"));
    if (start.size() != 2) {
      BOOST_THROW_EXCEPTION(fw::Exception()
          << fw::message_error_info("<player> node has invalid 'start' attribute."));
    }

    player_starts_[player_no] = fw::Vector(start[0], 0.0f, start[1]);
  }
}

Terrain *WorldReader::create_terrain(int width, int length, float* height_data) {
  return new Terrain(width, length, height_data);
}

Terrain *WorldReader::get_terrain() {
  if (terrain_ == nullptr) {
    BOOST_THROW_EXCEPTION(fw::Exception()
        << fw::message_error_info("Terrain has not been created yet!"));
  }

  return terrain_;
}

}

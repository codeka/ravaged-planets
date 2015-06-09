#include <memory>

#include <framework/bitmap.h>
#include <framework/framework.h>
#include <framework/logging.h>
#include <framework/graphics.h>
#include <framework/misc.h>
#include <framework/exception.h>
#include <framework/texture.h>
#include <framework/xml.h>

#include <game/world/world_reader.h>
#include <game/world/world.h>
#include <game/world/world_vfs.h>
#include <game/world/terrain.h>

namespace rp {

world_reader::world_reader() :
    _terrain(nullptr) {
}

world_reader::~world_reader() {
}

void world_reader::read(std::string name) {
  world_vfs vfs;
  world_file wf = vfs.open_file(name, false);

  int version;
  int trn_width;
  int trn_length;

  world_file_entry wfe = wf.get_entry("heightfield");
  wfe.read(&version, sizeof(int));
  if (version != 1) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("unknown terrain version"));
  }

  wfe.read(&trn_width, sizeof(int));
  wfe.read(&trn_length, sizeof(int));

  float *height_data = new float[trn_width * trn_length];
  wfe.read(height_data, trn_width * trn_length * sizeof(float));

  _name = name;
  _terrain = create_terrain(trn_width, trn_length);
  _terrain->_heights = height_data;

  for (int patch_z = 0; patch_z < _terrain->get_patches_length(); patch_z++) {
    for (int patch_x = 0; patch_x < _terrain->get_patches_width(); patch_x++) {
      std::string name = (boost::format("splatt-%1%-%2%.png") % patch_x % patch_z).str();
      wfe = wf.get_entry(name);

      fw::bitmap splatt(wfe.get_full_path().c_str());
      _terrain->set_splatt(patch_x, patch_z, splatt);
    }
  }

  wfe = wf.get_entry("minimap.png");
  if (wfe.exists()) {
    wfe.close();

    std::shared_ptr<fw::texture> tex(new fw::texture());
    tex->create(wfe.get_full_path());
    _minimap_background = tex;
  }

  wfe = wf.get_entry(name + ".wwmap");
  if (wfe.exists()) {
    wfe.close();

    fw::xml_element root(fw::load_xml(wfe.get_full_path(), "mapdesc", 1));
    read_wwmap(root);
  }

  wfe = wf.get_entry("collision_data");
  if (wfe.exists()) {
    read_collision_data(wfe);
  }
}

void world_reader::read_collision_data(world_file_entry &wfe) {
  int version;
  wfe.read(&version, sizeof(int));
  if (version != 1) {
    BOOST_THROW_EXCEPTION(fw::exception()
        << fw::message_error_info("unknown collision_data version"));
  }

  int width, length;
  wfe.read(&width, sizeof(int));
  wfe.read(&length, sizeof(int));

  for (int i = 0; i < (width * length); i++) {
    uint8_t n;
    wfe.read(&n, sizeof(uint8_t));
    _terrain->_collision_data.push_back(n != 0);
  }
}

void world_reader::read_wwmap(fw::xml_element root) {
  for (fw::xml_element child = root.get_first_child(); child.is_valid(); child =
      child.get_next_sibling()) {
    if (child.get_value() == "description") {
      _description = child.get_text();
    } else if (child.get_value() == "author") {
      _author = child.get_text();
    } else if (child.get_value() == "size") {
    } else if (child.get_value() == "players") {
      read_wwmap_players(child);
    } else {
      BOOST_THROW_EXCEPTION(fw::exception()
          << fw::message_error_info("Unknown child element of <mapdesc> node!"));
    }
  }
}

void world_reader::read_wwmap_players(fw::xml_element players_node) {
  for (fw::xml_element child = players_node.get_first_child(); child.is_valid();
      child = child.get_next_sibling()) {
    if (child.get_value() != "player") {
      BOOST_THROW_EXCEPTION(fw::exception()
          << fw::message_error_info("Unknown child element of <players> node!"));
    }

    int player_no = boost::lexical_cast<int>(child.get_attribute("no"));
    std::vector<float> start = fw::split<float>(child.get_attribute("start"));
    if (start.size() != 2) {
      BOOST_THROW_EXCEPTION(fw::exception()
          << fw::message_error_info("<player> node has invalid 'start' attribute."));
    }

    _player_starts[player_no] = fw::vector(start[0], 0.0f, start[1]);
  }
}

terrain *world_reader::create_terrain(int width, int length) {
  terrain *t = new terrain();
  t->create(width, length, false);
  return t;
}

terrain *world_reader::get_terrain() {
  if (_terrain == nullptr) {
    BOOST_THROW_EXCEPTION(fw::exception()
        << fw::message_error_info("Terrain has not been created yet!"));
  }

  return _terrain;
}

}

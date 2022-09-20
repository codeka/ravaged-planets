#include <framework/particle_config.h>
#include <framework/particle_emitter.h>
#include <framework/texture.h>
#include <framework/exception.h>
#include <framework/framework.h>
#include <framework/misc.h>
#include <framework/paths.h>
#include <framework/vector.h>
#include <framework/xml.h>
#include <framework/logging.h>

namespace fs = boost::filesystem;

namespace fw {

//-------------------------------------------------------------------------
class particle_effect_config_cache {
private:
  typedef std::map<std::string, std::shared_ptr<particle_effect_config> > config_map;
  config_map _configs;
public:
  std::shared_ptr<particle_effect_config> get_config(std::string const &name);
  void add_config(std::string const &name, std::shared_ptr<particle_effect_config> config);
};

std::shared_ptr<particle_effect_config> particle_effect_config_cache::get_config(std::string const &name) {
  config_map::iterator it = _configs.find(name);
  if (it == _configs.end())
    return std::shared_ptr<particle_effect_config>();

  return it->second;
}

void particle_effect_config_cache::add_config(std::string const &name, std::shared_ptr<particle_effect_config> config) {
  _configs[name] = config;
}

static particle_effect_config_cache g_cache;

//-------------------------------------------------------------------------
particle_effect_config::particle_effect_config() {
}

std::shared_ptr<particle_effect_config> particle_effect_config::load(std::string const &name) {
  fs::path filepath;
  if (fs::is_regular_file(name)) {
    filepath = name;
  } else {
    // first check whether the effect has been cached
    std::shared_ptr<particle_effect_config> config = g_cache.get_config(name);
    if (config) {
      return config;
    }

    filepath = fw::resolve("particles/" + name + ".part").string();
  }
  fw::xml_element xmldoc = fw::load_xml(filepath, "particle", 1);

  std::shared_ptr<particle_effect_config> config(new particle_effect_config());
  if (config->load_document(xmldoc)) {
    g_cache.add_config(name, config);
    return config;
  }

  return std::shared_ptr<particle_effect_config>();
}

// loads from the root node of the document
bool particle_effect_config::load_document(xml_element const &root) {
  try {
    for (xml_element child = root.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
      if (child.get_value() == "emitter") {
        load_emitter(child);
      } else {
        BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("unknown child element of <wwparticle>"));
      }
    }

    return true;
  } catch (fw::Exception const &e) {
    // if we get an exception, we'll just report the error and return false. it means we weren't able to parse the file.
    debug << "ERROR: could not parse .part file:" << std::endl;
    debug << diagnostic_information(e) << std::endl;
    return false;
  }
}

void particle_effect_config::load_emitter(xml_element const &elem) {
  std::shared_ptr<particle_emitter_config> emitter_config(new particle_emitter_config());
  emitter_config->load_emitter(elem);

  _emitter_configs.push_back(emitter_config);
}

//-------------------------------------------------------------------------
fw::Vector particle_emitter_config::spherical_location::get_point() const {
  fw::Vector random(fw::random() - 0.5f, fw::random() - 0.5f, fw::random() - 0.5f);

  // TODO: this is the implementation of "constant"
  return centre + (random.normalize() * radius);
}

//-------------------------------------------------------------------------
particle_emitter_config::life_state::life_state() {
  age = 0.0f;
  size.min = size.max = 1.0f;
  rotation_speed.min = rotation_speed.max = 0.0f;
  rotation_kind = rotation_kind::random;
  speed.min = speed.max = 0.0f;
  direction.min = direction.max = fw::Vector(0, 0, 0);
  alpha = 1.0f;
  color_row = 0;
  gravity.min = gravity.max = 0.0f;
}

particle_emitter_config::life_state::life_state(particle_emitter_config::life_state const &copy) {
  age = copy.age;
  size.min = copy.size.min;
  size.max = copy.size.max;
  rotation_speed.min = copy.rotation_speed.min;
  rotation_speed.max = copy.rotation_speed.max;
  rotation_kind = copy.rotation_kind;
  speed.min = copy.speed.min;
  speed.max = copy.speed.max;
  direction.min = copy.direction.min;
  direction.max = copy.direction.max;
  color_row = copy.color_row;
  alpha = copy.alpha;
  gravity.min = copy.gravity.min;
  gravity.max = copy.gravity.max;
}

//-------------------------------------------------------------------------
particle_emitter_config::particle_emitter_config() {
  emit_policy_value = 0.0f;

  position.centre = fw::Vector(0, 0, 0);
  position.radius = 0.0f;
  position.falloff = particle_emitter_config::constant;

  billboard.texture = std::shared_ptr<Texture>();
  billboard.mode = particle_emitter_config::normal;

  max_age.min = 4.0f;
  max_age.max = 4.0f;

  start_time = 0.0f;
  end_time = 0.0f;

  initial_count = 0;
}

void particle_emitter_config::load_emitter(xml_element const &emitter_elem) {
  if (emitter_elem.is_attribute_defined("start")) {
    start_time = emitter_elem.get_attribute<float>("start");
  }

  if (emitter_elem.is_attribute_defined("end")) {
    end_time = emitter_elem.get_attribute<float>("end");
  }

  if (emitter_elem.is_attribute_defined("initial")) {
    initial_count = emitter_elem.get_attribute<int>("initial");
  }

  for (xml_element child = emitter_elem.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
    if (child.get_value() == "position") {
      load_position(child);
    } else if (child.get_value() == "billboard") {
      load_billboard(child);
    } else if (child.get_value() == "life") {
      load_life(child);
    } else if (child.get_value() == "age") {
      parse_random_float(max_age, child);
    } else if (child.get_value() == "emit") {
      load_emit_policy(child);
    } else {
      BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("unknown child element of <emitter>"));
    }
  }
}

void particle_emitter_config::load_position(xml_element const &elem) {
  std::vector<float> components = fw::split<float>(elem.get_attribute("offset"));
  if (components.size() != 3) {
    BOOST_THROW_EXCEPTION(fw::Exception()
        << fw::message_error_info("'offset' attribute requires 3 floating point values"));
  }
  position.centre = fw::Vector(components[0], components[1], components[2]);

  if (elem.is_attribute_defined("radius")) {
    position.radius = boost::lexical_cast<float>(elem.get_attribute("radius"));
  }

  if (elem.is_attribute_defined("falloff")) {
    std::string value = elem.get_attribute("falloff");
    if (value == "linear") {
      position.falloff = particle_emitter_config::linear;
    } else if (value == "constant") {
      position.falloff = particle_emitter_config::constant;
    } else if (value == "exponential") {
      position.falloff = particle_emitter_config::exponential;
    } else {
      BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("unknown 'falloff' attribute value"));
    }
  }
}

void particle_emitter_config::load_billboard(xml_element const &elem) {
  std::string filename = elem.get_attribute("texture");
  std::shared_ptr<fw::Texture> texture(new fw::Texture());
  texture->create(fw::resolve("particles/" + filename));
  billboard.texture = texture;

  if (elem.is_attribute_defined("mode")) {
    std::string mode = elem.get_attribute("mode");
    if (mode == "additive") {
      billboard.mode = particle_emitter_config::additive;
    } else if (mode != "normal") {
      BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("Billboard mode must be 'additive' or 'normal'"));
    }
  }

  for (fw::xml_element child = elem.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
    if (child.get_value() == "area") {
      std::vector<float> components = fw::split<float>(child.get_attribute("rect"));
      if (components.size() != 4) {
        BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("rect values require 4 floating point values"));
      }

      billboard_rect rect;
      rect.left = components[0];
      rect.top = components[1];
      rect.right = components[2];
      rect.bottom = components[3];
      billboard.areas.push_back(rect);
    } else {
      BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("Unknown child of 'billboard'."));
    }
  }
}

void particle_emitter_config::load_life(xml_element const &elem) {
  life_state last_state;
  for (xml_element child = elem.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
    if (child.get_value() == "state") {
      life_state state(last_state);
      parse_life_state(state, child);
      life.push_back(state);
      last_state = state;
    } else {
      BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("unknown child element of <life>"));
    }
  }
}

void particle_emitter_config::load_emit_policy(xml_element const &elem) {
  emit_policy_name = elem.get_attribute("policy");
  if (elem.is_attribute_defined("value"))
    emit_policy_value = elem.get_attribute<float>("value");
  else
    emit_policy_value = 0.0f;
}

void particle_emitter_config::parse_life_state(life_state &state, xml_element const &elem) {
  state.age = boost::lexical_cast<float>(elem.get_attribute("age"));

  for (xml_element child = elem.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
    if (child.get_value() == "size") {
      parse_random_float(state.size, child);
    } else if (child.get_value() == "color") {
      if (child.is_attribute_defined("alpha")) {
        state.alpha = child.get_attribute<float>("alpha");
      }
      if (child.is_attribute_defined("row")) {
        state.color_row = child.get_attribute<int>("row");
      }
    } else if (child.get_value() == "rotation") {
      if (child.is_attribute_defined("kind")) {
        std::string kind = child.get_attribute("kind");
        if (kind == "direction") {
          state.rotation_kind = rotation_kind::direction;
        } else if (kind != "random") {
          BOOST_THROW_EXCEPTION(
              fw::Exception() << fw::message_error_info("kind expected to be 'random' or 'direction'"));
        }
      }

      if (child.is_attribute_defined("min")) {
        parse_random_float(state.rotation_speed, child);
      }
    } else if (child.get_value() == "gravity") {
      parse_random_float(state.gravity, child);
    } else if (child.get_value() == "speed") {
      parse_random_float(state.speed, child);
    } else if (child.get_value() == "direction") {
      parse_random_vector(state.direction, child);
    } else {
      BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("unknown child element of <state>"));
    }
  }
}

void particle_emitter_config::parse_random_float(random<float> &value, xml_element const &elem) {
  value.min = boost::lexical_cast<float>(elem.get_attribute("min"));
  value.max = boost::lexical_cast<float>(elem.get_attribute("max"));
}

void particle_emitter_config::parse_random_color(random<fw::Color> &value, xml_element const &elem) {
  std::vector<float> min_components = fw::split<float>(elem.get_attribute("min"));
  std::vector<float> max_components = fw::split<float>(elem.get_attribute("max"));

  if (min_components.size() == 3)
    value.min = fw::Color(min_components[0], min_components[1], min_components[2]);
  else if (min_components.size() == 4)
    value.min = fw::Color(min_components[0], min_components[1], min_components[2], min_components[3]);
  else {
    BOOST_THROW_EXCEPTION(
        fw::Exception() << fw::message_error_info("color values require 3 or 4 floating point values"));
  }

  if (max_components.size() == 3)
    value.max = fw::Color(max_components[0], max_components[1], max_components[2]);
  else if (max_components.size() == 4)
    value.max = fw::Color(max_components[0], max_components[1], max_components[2], min_components[3]);
  else {
    BOOST_THROW_EXCEPTION(
        fw::Exception() << fw::message_error_info("color values require 3 or 4 floating point values"));
  }
}

void particle_emitter_config::parse_random_vector(random<fw::Vector> &value, xml_element const &elem) {
  std::vector<float> min_components = fw::split<float>(elem.get_attribute("min"));
  std::vector<float> max_components = fw::split<float>(elem.get_attribute("max"));

  if (min_components.size() == 3)
    value.min = fw::Vector(min_components[0], min_components[1], min_components[2]);
  else {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("values require 3 floating point values"));
  }

  if (max_components.size() == 3)
    value.max = fw::Vector(max_components[0], max_components[1], max_components[2]);
  else {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("values require 3 floating point values"));
  }
}

}

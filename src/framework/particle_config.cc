#include <filesystem>

#include <framework/particle_config.h>
#include <framework/particle_emitter.h>
#include <framework/exception.h>
#include <framework/framework.h>
#include <framework/logging.h>
#include <framework/math.h>
#include <framework/misc.h>
#include <framework/paths.h>
#include <framework/texture.h>
#include <framework/xml.h>

namespace fs = std::filesystem;

namespace fw {

//-------------------------------------------------------------------------
class ParticleEffectConfigCache {
private:
  typedef std::map<std::string, std::shared_ptr<ParticleEffectConfig> > config_map;
  config_map _configs;
public:
  std::shared_ptr<ParticleEffectConfig> get_config(std::string const &name);
  void add_config(std::string const &name, std::shared_ptr<ParticleEffectConfig> config);
};

std::shared_ptr<ParticleEffectConfig> ParticleEffectConfigCache::get_config(std::string const &name) {
  config_map::iterator it = _configs.find(name);
  if (it == _configs.end())
    return std::shared_ptr<ParticleEffectConfig>();

  return it->second;
}

void ParticleEffectConfigCache::add_config(std::string const &name, std::shared_ptr<ParticleEffectConfig> config) {
  _configs[name] = config;
}

static ParticleEffectConfigCache g_cache;

//-------------------------------------------------------------------------
ParticleEffectConfig::ParticleEffectConfig() {
}

std::shared_ptr<ParticleEffectConfig> ParticleEffectConfig::load(std::string const &name) {
  fs::path filepath;
  if (fs::is_regular_file(name)) {
    filepath = name;
  } else {
    // first check whether the effect has been cached
    std::shared_ptr<ParticleEffectConfig> config = g_cache.get_config(name);
    if (config) {
      return config;
    }

    filepath = fw::resolve("particles/" + name + ".part").string();
  }
  fw::XmlElement xmldoc = fw::load_xml(filepath, "particle", 1);

  std::shared_ptr<ParticleEffectConfig> config(new ParticleEffectConfig());
  if (config->load_document(xmldoc)) {
    g_cache.add_config(name, config);
    return config;
  }

  return std::shared_ptr<ParticleEffectConfig>();
}

// loads from the root Node of the document
bool ParticleEffectConfig::load_document(XmlElement const &root) {
  try {
    for (XmlElement child = root.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
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

void ParticleEffectConfig::load_emitter(XmlElement const &elem) {
  std::shared_ptr<ParticleEmitterConfig> emitter_config(new ParticleEmitterConfig());
  emitter_config->load_emitter(elem);

  emitter_configs_.push_back(emitter_config);
}

//-------------------------------------------------------------------------
fw::Vector ParticleEmitterConfig::SphericalLocation::get_point() const {
  fw::Vector random(fw::random() - 0.5f, fw::random() - 0.5f, fw::random() - 0.5f);

  // TODO: this is the implementation of "constant"
  return center + (random.normalized() * radius);
}

//-------------------------------------------------------------------------
ParticleEmitterConfig::LifeState::LifeState() {
  age = 0.0f;
  size.min = size.max = 1.0f;
  rotation_speed.min = rotation_speed.max = 0.0f;
  rotation = ParticleRotation::kRandom;
  speed.min = speed.max = 0.0f;
  direction.min = direction.max = fw::Vector(0, 0, 0);
  alpha = 1.0f;
  color_row = 0;
  gravity.min = gravity.max = 0.0f;
}

ParticleEmitterConfig::LifeState::LifeState(ParticleEmitterConfig::LifeState const &copy) {
  age = copy.age;
  size.min = copy.size.min;
  size.max = copy.size.max;
  rotation_speed.min = copy.rotation_speed.min;
  rotation_speed.max = copy.rotation_speed.max;
  rotation = copy.rotation;
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
ParticleEmitterConfig::ParticleEmitterConfig() {
  emit_policy_value = 0.0f;

  position.center = fw::Vector(0, 0, 0);
  position.radius = 0.0f;
  position.falloff = ParticleEmitterConfig::kConstant;

  billboard.texture = std::shared_ptr<Texture>();
  billboard.mode = ParticleEmitterConfig::kNormal;

  max_age.min = 4.0f;
  max_age.max = 4.0f;

  start_time = 0.0f;
  end_time = 0.0f;

  initial_count = 0;
}

void ParticleEmitterConfig::load_emitter(XmlElement const &emitter_elem) {
  if (emitter_elem.is_attribute_defined("start")) {
    start_time = emitter_elem.get_attribute<float>("start");
  }

  if (emitter_elem.is_attribute_defined("end")) {
    end_time = emitter_elem.get_attribute<float>("end");
  }

  if (emitter_elem.is_attribute_defined("initial")) {
    initial_count = emitter_elem.get_attribute<int>("initial");
  }

  for (XmlElement child = emitter_elem.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
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

void ParticleEmitterConfig::load_position(XmlElement const &elem) {
  std::vector<float> components = fw::split<float>(elem.get_attribute("offset"));
  if (components.size() != 3) {
    BOOST_THROW_EXCEPTION(fw::Exception()
        << fw::message_error_info("'offset' attribute requires 3 floating point values"));
  }
  position.center = fw::Vector(components[0], components[1], components[2]);

  if (elem.is_attribute_defined("radius")) {
    position.radius = boost::lexical_cast<float>(elem.get_attribute("radius"));
  }

  if (elem.is_attribute_defined("falloff")) {
    std::string ParticleRotation = elem.get_attribute("falloff");
    if (ParticleRotation == "linear") {
      position.falloff = ParticleEmitterConfig::kLinear;
    } else if (ParticleRotation == "constant") {
      position.falloff = ParticleEmitterConfig::kConstant;
    } else if (ParticleRotation == "exponential") {
      position.falloff = ParticleEmitterConfig::kExponential;
    } else {
      BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("unknown 'falloff' attribute value"));
    }
  }
}

void ParticleEmitterConfig::load_billboard(XmlElement const &elem) {
  std::string filename = elem.get_attribute("texture");
  std::shared_ptr<fw::Texture> texture(new fw::Texture());
  texture->create(fw::resolve("particles/" + filename));
  billboard.texture = texture;

  if (elem.is_attribute_defined("mode")) {
    std::string mode = elem.get_attribute("mode");
    if (mode == "additive") {
      billboard.mode = ParticleEmitterConfig::kAdditive;
    } else if (mode != "normal") {
      BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("Billboard mode must be 'additive' or 'normal'"));
    }
  }

  for (fw::XmlElement child = elem.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
    if (child.get_value() == "area") {
      std::vector<float> components = fw::split<float>(child.get_attribute("rect"));
      if (components.size() != 4) {
        BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("rect values require 4 floating point values"));
      }

      Rectangle<float> rect;
      rect.left = components[0];
      rect.top = components[1];
      rect.width = components[2] - components[0];
      rect.height = components[3] - components[1];
      billboard.areas.push_back(rect);
    } else {
      BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("Unknown child of 'billboard'."));
    }
  }
}

void ParticleEmitterConfig::load_life(XmlElement const &elem) {
  LifeState last_state;
  for (XmlElement child = elem.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
    if (child.get_value() == "state") {
      LifeState state(last_state);
      parse_life_state(state, child);
      life.push_back(state);
      last_state = state;
    } else {
      BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("unknown child element of <life>"));
    }
  }
}

void ParticleEmitterConfig::load_emit_policy(XmlElement const &elem) {
  emit_policy_name = elem.get_attribute("policy");
  if (elem.is_attribute_defined("value"))
    emit_policy_value = elem.get_attribute<float>("value");
  else
    emit_policy_value = 0.0f;
}

void ParticleEmitterConfig::parse_life_state(LifeState &state, XmlElement const &elem) {
  state.age = boost::lexical_cast<float>(elem.get_attribute("age"));

  for (XmlElement child = elem.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
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
          state.rotation = ParticleRotation::kDirection;
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

void ParticleEmitterConfig::parse_random_float(Random<float> &ParticleRotation, XmlElement const &elem) {
  ParticleRotation.min = boost::lexical_cast<float>(elem.get_attribute("min"));
  ParticleRotation.max = boost::lexical_cast<float>(elem.get_attribute("max"));
}

void ParticleEmitterConfig::parse_random_color(Random<fw::Color> &ParticleRotation, XmlElement const &elem) {
  std::vector<float> min_components = fw::split<float>(elem.get_attribute("min"));
  std::vector<float> max_components = fw::split<float>(elem.get_attribute("max"));

  if (min_components.size() == 3)
    ParticleRotation.min = fw::Color(min_components[0], min_components[1], min_components[2]);
  else if (min_components.size() == 4)
    ParticleRotation.min = fw::Color(min_components[0], min_components[1], min_components[2], min_components[3]);
  else {
    BOOST_THROW_EXCEPTION(
        fw::Exception() << fw::message_error_info("color values require 3 or 4 floating point values"));
  }

  if (max_components.size() == 3)
    ParticleRotation.max = fw::Color(max_components[0], max_components[1], max_components[2]);
  else if (max_components.size() == 4)
    ParticleRotation.max = fw::Color(max_components[0], max_components[1], max_components[2], min_components[3]);
  else {
    BOOST_THROW_EXCEPTION(
        fw::Exception() << fw::message_error_info("color values require 3 or 4 floating point values"));
  }
}

void ParticleEmitterConfig::parse_random_vector(Random<fw::Vector> &ParticleRotation, XmlElement const &elem) {
  std::vector<float> min_components = fw::split<float>(elem.get_attribute("min"));
  std::vector<float> max_components = fw::split<float>(elem.get_attribute("max"));

  if (min_components.size() == 3)
    ParticleRotation.min = fw::Vector(min_components[0], min_components[1], min_components[2]);
  else {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("values require 3 floating point values"));
  }

  if (max_components.size() == 3)
    ParticleRotation.max = fw::Vector(max_components[0], max_components[1], max_components[2]);
  else {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("values require 3 floating point values"));
  }
}

}

#include <framework/particle_config.h>

#include <filesystem>

#include <absl/strings/numbers.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/str_split.h>

#include <framework/particle_emitter.h>
#include <framework/exception.h>
#include <framework/framework.h>
#include <framework/logging.h>
#include <framework/math.h>
#include <framework/misc.h>
#include <framework/paths.h>
#include <framework/status.h>
#include <framework/texture.h>
#include <framework/xml.h>

namespace fs = std::filesystem;

namespace fw {
namespace {

class ParticleEffectConfigCache {
private:
  typedef std::map<std::string, std::shared_ptr<ParticleEffectConfig> > config_map;
  config_map _configs;
public:
  std::shared_ptr<ParticleEffectConfig> GetConfig(std::string_view name);
  void AddConfig(std::string_view name, std::shared_ptr<ParticleEffectConfig> config);
};

std::shared_ptr<ParticleEffectConfig> ParticleEffectConfigCache::GetConfig(std::string_view name) {
  config_map::iterator it = _configs.find(std::string(name));
  if (it == _configs.end())
    return std::shared_ptr<ParticleEffectConfig>();

  return it->second;
}

void ParticleEffectConfigCache::AddConfig(
    std::string_view name, std::shared_ptr<ParticleEffectConfig> config) {
  _configs[std::string(name)] = config;
}

static ParticleEffectConfigCache g_cache;

}  // namespace

ParticleEffectConfig::ParticleEffectConfig() {
}

fw::StatusOr<std::shared_ptr<ParticleEffectConfig>> ParticleEffectConfig::Load(
    std::string_view name) {
  fs::path filepath;
  if (fs::is_regular_file(name)) {
    filepath = name;
  } else {
    // first check whether the effect has been cached
    std::shared_ptr<ParticleEffectConfig> config = g_cache.GetConfig(name);
    if (config) {
      return config;
    }

    filepath = fw::resolve(absl::StrCat("particles/", name, ".part")).string();
  }
  ASSIGN_OR_RETURN(fw::XmlElement xmldoc, fw::LoadXml(filepath, "particle", 1));

  auto config = std::make_shared<ParticleEffectConfig>();
  RETURN_IF_ERROR(config->LoadDocument(xmldoc));
  g_cache.AddConfig(name, config);
  return config;
}

// loads from the root Node of the document
fw::Status ParticleEffectConfig::LoadDocument(XmlElement const &root) {
  for (XmlElement child : root.children()) {
    if (child.get_value() == "emitter") {
      RETURN_IF_ERROR(LoadEmitter(child));
    } else {
      return fw::ErrorStatus("unknown child element of <wwparticle>: ") << child.get_value();
    }
  }

  return fw::OkStatus();
}

fw::Status ParticleEffectConfig::LoadEmitter(XmlElement const &elem) {
  std::shared_ptr<ParticleEmitterConfig> emitter_config(new ParticleEmitterConfig());
  RETURN_IF_ERROR(emitter_config->LoadEmitter(elem));

  emitter_configs_.push_back(emitter_config);
  return fw::OkStatus();
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

fw::Status ParticleEmitterConfig::LoadEmitter(XmlElement const &emitter_elem) {
  auto start_time = emitter_elem.GetAttributef<float>("start");
  if (start_time.ok()) {
    this->start_time = *start_time;
  }

  auto end_time = emitter_elem.GetAttributef<float>("end");
  if (end_time.ok()) {
    this->end_time = *end_time;
  }

  auto initial_count = emitter_elem.GetAttributei<int>("initial");
  if (initial_count.ok()) {
    initial_count = *initial_count;
  }

  for (XmlElement child : emitter_elem.children()) {
    if (child.get_value() == "position") {
      RETURN_IF_ERROR(LoadPosition(child));
    } else if (child.get_value() == "billboard") {
      RETURN_IF_ERROR(LoadBillboard(child));
    } else if (child.get_value() == "life") {
      RETURN_IF_ERROR(LoadLife(child));
    } else if (child.get_value() == "age") {
      ASSIGN_OR_RETURN(max_age, ParseRandomFloat(child));
    } else if (child.get_value() == "emit") {
      RETURN_IF_ERROR(LoadEmitPolicy(child));
    } else {
      return fw::ErrorStatus("unknown child element of <emitter>: ") << child.get_value();
    }
  }
  return fw::OkStatus();
}

fw::Status ParticleEmitterConfig::LoadPosition(XmlElement const &elem) {
  ASSIGN_OR_RETURN(std::string offset, elem.GetAttribute("offset"));
  std::vector<std::string> components = absl::StrSplit(offset, " ");
  if (components.size() != 3) {
    return fw::ErrorStatus("'offset' attribute requires 3 floating point values") << offset;
  }
  position.center =
      fw::Vector(std::stoi(components[0]), std::stoi(components[1]), std::stoi(components[2]));

  auto radius = elem.GetAttributef<float>("radius");
  if (radius.ok()) {
    position.radius = *radius;
  }

  auto falloff = elem.GetAttribute("falloff");
  if (falloff.ok()) {
    if (*falloff == "linear") {
      position.falloff = ParticleEmitterConfig::kLinear;
    } else if (*falloff == "constant") {
      position.falloff = ParticleEmitterConfig::kConstant;
    } else if (*falloff == "exponential") {
      position.falloff = ParticleEmitterConfig::kExponential;
    } else {
      return fw::ErrorStatus("unknown 'falloff' attribute value: ") << *falloff;
    }
  }

  return fw::OkStatus();
}

fw::Status ParticleEmitterConfig::LoadBillboard(XmlElement const &elem) {
  ASSIGN_OR_RETURN(auto filename, elem.GetAttribute("texture"));

  std::shared_ptr<fw::Texture> texture(new fw::Texture());
  texture->create(fw::resolve("particles/" + filename));
  billboard.texture = texture;

  auto mode = elem.GetAttribute("mode");
  if (mode.ok()) {
    if (*mode == "additive") {
      billboard.mode = ParticleEmitterConfig::kAdditive;
    } else if (*mode != "normal") {
      return fw::ErrorStatus("billboard mode must be 'additive' or 'normal': ") << *mode;
    }
  }

  for (fw::XmlElement child : elem.children()) {
    if (child.get_value() == "area") {
      auto rect_attr = child.GetAttribute("rect");
      if (!rect_attr.ok()) {
        return fw::ErrorStatus("'rect' attribute is required on <area> element");
      }
      std::vector<std::string> components = absl::StrSplit(*rect_attr, " ");
      if (components.size() != 4) {
        return fw::ErrorStatus("rect values require 4 floating point values: ") << *rect_attr;
      }

      Rectangle<float> rect;
      rect.left = std::stof(components[0]);
      rect.top = std::stof(components[1]);
      rect.width = std::stof(components[2]) - rect.left;
      rect.height = std::stof(components[3]) - rect.top;
      billboard.areas.push_back(rect);
    } else {
      return fw::ErrorStatus("unknown child of 'billboard': ") << child.get_value();
    }
  }
  return fw::OkStatus();
}

fw::Status ParticleEmitterConfig::LoadLife(XmlElement const &elem) {
  LifeState last_state;
  for (XmlElement child : elem.children()) {
    if (child.get_value() == "state") {
      ASSIGN_OR_RETURN(auto state, ParseLifeState(child, last_state));
      life.push_back(state);
      last_state = state;
    } else {
      return fw::ErrorStatus("unknown child element of <life>: ") << elem.get_value();
    }
  }
  return fw::OkStatus();
}

fw::Status ParticleEmitterConfig::LoadEmitPolicy(XmlElement const &elem) {
  ASSIGN_OR_RETURN(emit_policy_name, elem.GetAttribute("policy"));
  auto value = elem.GetAttributef<float>("value");
  if (value.ok()) {
    emit_policy_value = *value;
  } else {
    emit_policy_value = 0.0f;
  }
  return fw::OkStatus();
}

fw::StatusOr<ParticleEmitterConfig::LifeState> ParticleEmitterConfig::ParseLifeState(
    XmlElement const &elem, std::optional<LifeState> last_life_state) {
  LifeState state = last_life_state.has_value() ? LifeState(*last_life_state) : LifeState();
  ASSIGN_OR_RETURN(state.age, elem.GetAttributef<float>("age"));

  for (auto child : elem.children()) {
    if (child.get_value() == "size") {
      ASSIGN_OR_RETURN(state.size, ParseRandomFloat(child));
    } else if (child.get_value() == "color") {
      auto alpha = child.GetAttributef<float>("alpha");
      if (alpha.ok()) {
        state.alpha = *alpha;
      }
      auto row = child.GetAttributei<int>("row");
      if (row.ok()) {
        state.color_row = *row;
      }
    } else if (child.get_value() == "rotation") {
      auto kind = child.GetAttribute("kind");
      if (kind.ok()) {
        if (*kind == "direction") {
          state.rotation = ParticleRotation::kDirection;
        } else if (*kind != "random") {
          return fw::ErrorStatus("kind expected to be 'random' or 'direction', not ") << *kind;
        }
      }

      auto min = child.GetAttribute("min");
      if (min.ok()) {
        ASSIGN_OR_RETURN(state.rotation_speed, ParseRandomFloat(child));
      }
    } else if (child.get_value() == "gravity") {
      ASSIGN_OR_RETURN(state.gravity, ParseRandomFloat(child));
    } else if (child.get_value() == "speed") {
      ASSIGN_OR_RETURN(state.speed, ParseRandomFloat(child));
    } else if (child.get_value() == "direction") {
      ASSIGN_OR_RETURN(state.direction, ParseRandomVector(child));
    } else {
      return fw::ErrorStatus("unknown child element of <state>:") << child.get_value();
    }
  }
  return fw::OkStatus();
}

fw::StatusOr<ParticleEmitterConfig::Random<float>> ParticleEmitterConfig::ParseRandomFloat(
    XmlElement const &elem) {
  ASSIGN_OR_RETURN(auto min, elem.GetAttributef<float>("min"));
  ASSIGN_OR_RETURN(auto max, elem.GetAttributef<float>("max"));
  return Random<float>(min, max);
}

fw::StatusOr<ParticleEmitterConfig::Random<Color>> ParticleEmitterConfig::ParseRandomColor(
    XmlElement const &elem) {
  ASSIGN_OR_RETURN(std::string min_attr, elem.GetAttribute("min"));
  ASSIGN_OR_RETURN(std::string max_attr, elem.GetAttribute("max"));
  std::vector<std::string> min_components = absl::StrSplit(min_attr, " ");
  std::vector<std::string> max_components = absl::StrSplit(max_attr, " ");

  auto value = Random<Color>();
  if (min_components.size() == 3) {
    float red, green, blue;
    if (!absl::SimpleAtof(min_components[0], &red)
        || !absl::SimpleAtof(min_components[1], &green)
        || !absl::SimpleAtof(min_components[2], &blue)) {
      return fw::ErrorStatus("invalid color: ") << min_attr;
    }
    value.min = fw::Color(red, green, blue);
  } else if (min_components.size() == 4) {
    float alpha, red, green, blue;
    if (!absl::SimpleAtof(min_components[0], &alpha)
        || !absl::SimpleAtof(min_components[1], &red)
        || !absl::SimpleAtof(min_components[2], &green)
        || !absl::SimpleAtof(min_components[2], &blue)) {
      return fw::ErrorStatus("invalid color: ") << min_attr;
    }
    value.min = fw::Color(alpha, red, green, blue);
  } else {
    return fw::ErrorStatus("color values require 3 or 4 floating point values, got: ") << min_attr;
  }

  if (max_components.size() == 3) {
    float red, green, blue;
    if (!absl::SimpleAtof(max_components[0], &red)
        || !absl::SimpleAtof(max_components[1], &green)
        || !absl::SimpleAtof(max_components[2], &blue)) {
      return fw::ErrorStatus("invalid color: ") << max_attr;
    }
    value.max = fw::Color(red, green, blue);
  } else if (max_components.size() == 4) {
    float alpha, red, green, blue;
    if (!absl::SimpleAtof(max_components[0], &alpha)
        || !absl::SimpleAtof(max_components[1], &red)
        || !absl::SimpleAtof(max_components[2], &green)
        || !absl::SimpleAtof(max_components[2], &blue)) {
      return fw::ErrorStatus("invalid color: ") << max_attr;
    }
    value.max = fw::Color(alpha, red, green, blue);
  } else {
    return fw::ErrorStatus("color values require 3 or 4 floating point values, got: ") << max_attr;
  }

  return value;
}

fw::StatusOr<ParticleEmitterConfig::Random<Vector>> ParticleEmitterConfig::ParseRandomVector(
    XmlElement const &elem) {
  ASSIGN_OR_RETURN(std::string min_attr, elem.GetAttribute("min"));
  ASSIGN_OR_RETURN(std::string max_attr, elem.GetAttribute("max"));
  std::vector<std::string> min_components = absl::StrSplit(min_attr, " ");
  std::vector<std::string> max_components = absl::StrSplit(max_attr, " ");

  auto value = Random<Vector>();
  if (min_components.size() == 3) {
    float x, y, z;
    if (!absl::SimpleAtof(min_components[0], &x)
        || !absl::SimpleAtof(min_components[1], &y)
        || !absl::SimpleAtof(min_components[2], &z)) {
      return fw::ErrorStatus("invalid vector: ") << min_attr;
    }
    value.min = Vector(x, y, z);
  } else {
    return fw::ErrorStatus("vector values require 3 floating point values, got: ") << min_attr;
  }

  if (max_components.size() == 3) {
    float x, y, z;
    if (!absl::SimpleAtof(max_components[0], &x)
        || !absl::SimpleAtof(max_components[1], &y)
        || !absl::SimpleAtof(max_components[2], &z)) {
      return fw::ErrorStatus("invalid color: ") << max_attr;
    }
    value.max = fw::Vector(x, y, z);
  } else {
    return fw::ErrorStatus("vector values require 3 floating point values, got: ") << max_attr;
  }

  return value;
}

}

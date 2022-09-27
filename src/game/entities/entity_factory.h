#pragma once

#include <optional>
#include <map>
#include <memory>
#include <vector>

#include <framework/lua.h>

#include <game/entities/entity_attribute.h>

namespace fw {
class XmlElement;
}

// This is a helper macro for registering component types with the entity_factory.
#define ENT_COMPONENT_REGISTER(name, type) \
  ent::component_register reg_ ## type(name, []() { return new type(); })

namespace ent {
class Entity;
class EntityComponent;

// this class is used to build entities from their XML definition file.
class EntityFactory {
private:
  void load_entities();

  EntityComponent *create_component(std::string component_type_name);
public:
  EntityFactory();
  ~EntityFactory();

  // populates the Entity with details for the given Entity name
  void populate(std::shared_ptr<Entity> ent, std::string name);

  // gets the template with the given name
  std::optional<fw::lua::Value> get_template(std::string name);

  // populates a vector with all of the Entity templates
  void get_templates(std::vector<fw::lua::Value> &templates);

  // helper method that populates a vector with entities that are buildable (and
  // in the given build_group)
  void get_buildable_templates(std::string const &build_group, std::vector<fw::lua::Value> &templates);
};

// this is a helper class that you use indirectly via the ENT_COMPONENT_REGISTER macro
// to register a component with the entity_factory.
class component_register {
public:
  component_register(char const *name, std::function<EntityComponent *()> fn);
};

}

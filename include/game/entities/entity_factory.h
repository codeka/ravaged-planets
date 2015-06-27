#pragma once

#include <map>
#include <memory>
#include <vector>

#include <game/entities/entity_attribute.h>

namespace fw {
class xml_element;
}

/** This is a helper macro for registering component types with the entity_factory. */
#define ENT_COMPONENT_REGISTER(name, type)								\
	ent::entity_component *create_ ## type () { return new type(); }	\
	ent::component_register reg_ ## type(name, create_ ## type)

namespace ent {
class entity;
class entity_component;

/** A description of a component, which we can query, or use to create the component. */
class entity_component_template {
public:
  entity_component_template();
  virtual ~entity_component_template();

  virtual bool entity_component_template::load(fw::xml_element const &elem);

  std::string name;
  int identifier;
  std::map<std::string, std::string> properties;
};

// this class contains the parsed .wwentity file which we'll use for creating (and describing)
// entities so we can create them (via the entity_factory)
class entity_template {
public:
  entity_template();
  virtual ~entity_template();

  std::string name;
  std::vector<std::shared_ptr<entity_component_template>> components;
  std::vector<entity_attribute> attributes;
};

// this class is used to build entities from their XML definition file.
class entity_factory {
private:
  void load_entities();
  void load_document(std::shared_ptr<entity_template> desc, fw::xml_element const &root);
  void load_components(std::shared_ptr<entity_template> desc, fw::xml_element const &elem);
  void load_component(std::shared_ptr<entity_template> desc, fw::xml_element const &elem);
  void load_attributes(std::shared_ptr<entity_template> desc, fw::xml_element const &elem);
  void load_attribute(std::shared_ptr<entity_template> desc, fw::xml_element const &elem);

  entity_component *create_component(std::string component_type_name);
public:
  entity_factory();
  ~entity_factory();

  // populates the entity with details for the given entity name
  void populate(std::shared_ptr<entity> ent, std::string name);

  // populates a vector with all of the entity templates
  void get_templates(std::vector<std::shared_ptr<entity_template>> &templates);

  // gets the template with the given name
  std::shared_ptr<entity_template> get_template(std::string name);

  // helper method that populates a vector with entities that are buildable (and
  // in the given build_group)
  void get_buildable_templates(std::string const &build_group,
      std::vector<std::shared_ptr<entity_template>> &templates);
};

// this is a helper class that you use indirectly via the ENT_COMPONENT_REGISTER macro
// to register a component with the entity_factory.
typedef entity_component *(*create_component_fn)();
class component_register {
public:
  component_register(char const *name, create_component_fn fn);
};

}

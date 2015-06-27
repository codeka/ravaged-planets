#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include <framework/xml.h>
#include <framework/logging.h>

#include <game/entities/entity.h>
#include <game/entities/entity_factory.h>
#include <game/entities/buildable_component.h>

namespace fs = boost::filesystem;

namespace ent {

typedef std::map<std::string, std::shared_ptr<entity_template> > entity_template_map;
static entity_template_map *entity_templates = 0;
static std::map<std::string, create_component_fn> *comp_registry = 0;

entity_factory::entity_factory() {
  if (entity_templates == nullptr)
    load_entities();
}

entity_factory::~entity_factory() {
}

void entity_factory::populate(std::shared_ptr<entity> ent, std::string name) {
  // first, find the template we'll use for creating the entity
  entity_template_map::iterator it = entity_templates->find(name);
  if (it == entity_templates->end()) {
    fw::debug << boost::format("  warning: unknown entity: %1%") % name << std::endl;
    return;
  }

  std::shared_ptr<entity_template> &entity_template = it->second;

  // add all of the attributes before we add any of the components
  BOOST_FOREACH(entity_attribute attr, entity_template->attributes) {
    ent->add_attribute(attr);
  }

  // then add all of the components as well
  BOOST_FOREACH(auto comp_template, entity_template->components) {
    entity_component *comp = create_component(comp_template->name);
    if (comp != nullptr) {
      comp->apply_template(comp_template);
      ent->add_component(comp);
      comp->set_entity(ent);
    }
  }
}

 // gets the complete list of entity_templates
void entity_factory::get_templates(std::vector<std::shared_ptr<entity_template>> &templates) {
  BOOST_FOREACH(auto &kvp, *entity_templates){
    std::shared_ptr<entity_template> &ent_template = kvp.second;
    templates.push_back(ent_template);
  }
}

std::shared_ptr<entity_template> entity_factory::get_template(std::string name) {
  entity_template_map::iterator it = entity_templates->find(name);
  if (it == entity_templates->end()) {
    return std::shared_ptr<entity_template>();
  }

  return it->second;
}

// helper method that populates a vector with entities that are buildable (and in the given build_group)
void entity_factory::get_buildable_templates(std::string const &build_group,
    std::vector<std::shared_ptr<entity_template>> &templates) {
  BOOST_FOREACH(entity_template_map::value_type & kvp, *entity_templates) {
    std::shared_ptr<entity_template> &ent_template = kvp.second;
    BOOST_FOREACH(auto comp_template, ent_template->components) {
      if (comp_template->identifier == buildable_component::identifier) {
        if (comp_template->properties["BuildGroup"] == build_group) {
          templates.push_back(ent_template);
          break;
        }
      }
    }
  }
}

 // loads all of the *.entity files in the .\data\entities folder one by one, and
 // registers them in the entity_template_map
void entity_factory::load_entities() {
  entity_templates = new entity_template_map();

  fs::path base_path = fs::initial_path() / "data/entities";
  fs::directory_iterator end_it;
  for (fs::directory_iterator it(base_path); it != end_it; ++it) {
    if (fs::is_regular_file(it->status()) && it->path().extension() == ".entity") {
      fw::xml_element root = fw::load_xml(it->path(), "entity", 1);
      std::shared_ptr<entity_template> ent_template(new entity_template());

      std::string ent_template_name = it->path().stem().string();
      ent_template->name = ent_template_name;

      load_document(ent_template, root);

      (*entity_templates)[ent_template_name] = ent_template;
    }
  }
}

void entity_factory::load_document(std::shared_ptr<entity_template> ent_template, fw::xml_element const &root) {
  for (fw::xml_element elem = root.get_first_child(); elem.is_valid(); elem = elem.get_next_sibling()) {
    if (elem.get_value() == "components") {
      load_components(ent_template, elem);
    } else if (elem.get_value() == "attributes") {
      load_attributes(ent_template, elem);
    } else {
      fw::debug << boost::format("  warning: skipping unknown child element \"%1%\"") % elem.get_value() << std::endl;
    }
  }
}

void entity_factory::load_components(std::shared_ptr<entity_template> ent_template, fw::xml_element const &elem) {
  for (fw::xml_element child = elem.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
    if (child.get_value() == "component") {
      load_component(ent_template, child);
    } else {
      fw::debug << boost::format("  warning: skipping unknown child element \"%1%\"") % child.get_value() << std::endl;
    }
  }
}

void entity_factory::load_component(std::shared_ptr<entity_template> ent_template, fw::xml_element const &elem) {
  std::string component_type_name = elem.get_attribute("type");

  entity_component *comp = create_component(component_type_name);
  if (comp == nullptr) {
    return;
  }

  std::shared_ptr<entity_component_template> comp_template(comp->create_template());
  comp_template->name = component_type_name;
  comp_template->identifier = comp->get_identifier();

  if (comp_template->load(elem)) {
    ent_template->components.push_back(comp_template);
  }
}

void entity_factory::load_attributes(std::shared_ptr<entity_template> ent_template, fw::xml_element const &elem) {
  for (fw::xml_element child = elem.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
    if (child.get_value() == "attribute") {
      load_attribute(ent_template, child);
    } else {
      fw::debug << boost::format("  warning: skipping unknown child element \"%1%\"") % child.get_value() << std::endl;
    }
  }
}

void entity_factory::load_attribute(std::shared_ptr<entity_template> ent_template, fw::xml_element const &elem) {
  std::string attr_name = elem.get_attribute("name");
  std::string attr_type = elem.get_attribute("type");

  boost::any attr_value;
  if (attr_type == "int") {
    attr_value = boost::any(elem.get_attribute<int>("value"));
  } else if (attr_type == "float") {
    attr_value = boost::any(elem.get_attribute<float>("value"));
  } else if (attr_type == "string") {
    attr_value = boost::any(elem.get_attribute("value"));
  } else {
    fw::debug << boost::format("  warning: skipping unknown attribute type \"%1%\"") % attr_type << std::endl;
  }

  entity_attribute attr(attr_name, attr_value);
  ent_template->attributes.push_back(attr);
}

entity_component *entity_factory::create_component(std::string component_type_name) {
  create_component_fn fn = (*comp_registry)[component_type_name];
  if (fn == 0) {
    fw::debug << boost::format("  warning: skipping unknown component \"%1%\"") % component_type_name << std::endl;
    return 0;
  }

  return fn();
}

//-------------------------------------------------------------------------
entity_component_template::entity_component_template() : identifier(-1) {
}

entity_component_template::~entity_component_template() {
}

bool entity_component_template::load(fw::xml_element const &elem) {
  for (fw::xml_element child = elem.get_first_child(); child.is_valid(); child = child.get_next_sibling()) {
    if (child.get_value() != "property") {
      // we don't write an error because the sub-class might've already done
      // something with this element.
      continue;
    }

    std::string name = child.get_attribute("name");
    std::string value = child.get_text();

    properties[name] = value;
  }

  return true;
}

//-------------------------------------------------------------------------
entity_template::entity_template() {
}

entity_template::~entity_template() {
}

//-------------------------------------------------------------------------
component_register::component_register(char const *name, create_component_fn fn) {
  if (comp_registry == nullptr) {
    comp_registry = new std::map<std::string, create_component_fn>();
  }

  (*comp_registry)[name] = fn;
}

}

#include <filesystem>

#include <framework/xml.h>
#include <framework/logging.h>
#include <framework/paths.h>

#include <game/entities/entity.h>
#include <game/entities/entity_factory.h>
#include <game/entities/buildable_component.h>

namespace fs = std::filesystem;

namespace ent {

typedef std::map<std::string, fw::lua::LuaContext *> entity_template_map;
static entity_template_map *entity_templates = nullptr;
static std::map<std::string, std::function<EntityComponent *()>> *comp_registry = nullptr;

EntityFactory::EntityFactory() {
  if (entity_templates == nullptr) {
    load_entities();
  }
}

EntityFactory::~EntityFactory() {
}

void EntityFactory::populate(std::shared_ptr<Entity> ent, std::string name) {
  // first, find the template we'll use for creating the Entity
  std::optional<fw::lua::Value> entity_template = get_template(name);
  if (!entity_template) {
    LOG(WARN) << "  unknown Entity: " << name;
    return;
  }

  // add all of the attributes before we add any of the components, as the components might want to refer to the
  // attributes.
  for (auto& kvp : *entity_template) {
    std::string key_name = kvp.key<std::string>();
    if (key_name == "components") {
      continue;
    }

    ent->add_attribute(EntityAttribute(key_name, kvp.value<boost::any>()));
  }

  // then add all of the components as well
  // TODO: support begin/end directly on IndexValue.
  fw::lua::Value components = (*entity_template)["components"];
  for (auto& kvp : components) {
    EntityComponent* component = create_component(kvp.key<std::string>());
    if (component != nullptr) {
      component->apply_template(kvp.value<fw::lua::Value>());
      ent->add_component(component);
      component->set_entity(ent);
    }
  }
}

std::optional<fw::lua::Value> EntityFactory::get_template(std::string name) {
  entity_template_map::iterator it = entity_templates->find(name);
  if (it == entity_templates->end()) {
    return std::nullopt;
  }

  fw::lua::LuaContext *ctx = it->second;
  return ctx->globals()["Entity"];
}

// gets the complete list of entity_templates
void EntityFactory::get_templates(std::vector<fw::lua::Value> &templates) {
  for(auto &kvp : *entity_templates) {
    fw::lua::LuaContext *ctx = kvp.second;
    templates.push_back(ctx->globals()["Entity"]);
  }
}

// helper method that populates a vector with entities that are buildable (and in the given build_group)
void EntityFactory::get_buildable_templates(std::string const &build_group,
      std::vector<fw::lua::Value> &templates) {
  for(entity_template_map::value_type &kvp : *entity_templates) {
    fw::lua::LuaContext *ctx = kvp.second;
    fw::lua::Value tmpl = ctx->globals()["Entity"];

    fw::lua::Value buildable_tmpl = tmpl["components"]["Buildable"];
    if (!buildable_tmpl.is_nil()) {
      if (buildable_tmpl["BuildGroup"].value<std::string>() == build_group) {
        templates.push_back(tmpl);
      }
    }
  }
}

 // loads all of the *.Entity files in the .\data\entities folder one by one, and
 // registers them in the entity_template_map
void EntityFactory::load_entities() {
  entity_templates = new entity_template_map();

  fs::path base_path = fw::install_base_path() / "entities";
  fs::directory_iterator end_it;
  for (fs::directory_iterator it(base_path); it != end_it; ++it) {
    if (fs::is_regular_file(it->status()) && it->path().extension() == ".entity") {
      std::string tmpl_name = it->path().stem().string();

      fw::lua::LuaContext* ctx = new fw::lua::LuaContext();
      ctx->load_script(it->path());

      fw::lua::Value tmpl = ctx->globals()["Entity"];
      tmpl["name"] = tmpl_name;

      // TODO: loop through components and register their identifier(?)
      (*entity_templates)[tmpl_name] = ctx;
    }
  }
}

EntityComponent *EntityFactory::create_component(std::string component_type_name) {
  std::function<EntityComponent *()> fn = (*comp_registry)[component_type_name];
  if (!fn) {
    LOG(WARN) << "  skipping unknown component: " << component_type_name;
    return nullptr;
  }

  return fn();
}

//-------------------------------------------------------------------------
component_register::component_register(char const *name, std::function<EntityComponent *()> fn) {
  if (comp_registry == nullptr) {
    comp_registry = new std::map<std::string, std::function<EntityComponent *()>>();
  }

  (*comp_registry)[name] = fn;
}

}

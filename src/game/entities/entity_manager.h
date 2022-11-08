#pragma once

#include <memory>

#include <framework/scenegraph.h>
#include <framework/math.h>

#include <game/entities/entity.h>

namespace fw {
class Graphics;
}

namespace ent {
class Entity;
class EntityDebug;
class PatchManager;

// Manages all the entities in the game, and contains various "indexes" of entities so that we
// can access them efficiently.
class EntityManager {
private:
  std::list<std::shared_ptr<Entity>> all_entities_;
  std::list<std::shared_ptr<Entity>> destroyed_entities_;
  std::list<std::weak_ptr<Entity>> selected_entities_;

  std::map<int, std::list<std::weak_ptr<Entity>>> entities_by_component_;

  EntityDebug *debug_;
  PatchManager *patch_mgr_;
  fw::Vector view_center_;

  // removes the destroyed entities from the various lists
  void cleanup_destroyed();

public:
  EntityManager();
  ~EntityManager();

  // initialize the EntityManager. this should be called after the
  // world has been created and the terrain is available as well..
  void initialize();

  // create a new Entity based on the given .Entity template
  std::shared_ptr<Entity> create_entity(std::string const &template_name, entity_id id);
  std::shared_ptr<Entity> create_entity(std::shared_ptr<Entity> created_by, std::string const &template_name, entity_id id);

  // destroy the given Entity (we actually remove it on the next update cycle)
  void destroy(std::weak_ptr<Entity> Entity);

  // gets the Entity with the given identifier
  std::weak_ptr<Entity> get_entity(entity_id id);

  // gets the Entity that's closest to the given ray (used for single selection and stuff)
  std::weak_ptr<Entity> get_entity(fw::Vector const &start, fw::Vector const &direction);

  // gets a reference to a list of all the entities with the component with the given identifier.
  std::list<std::weak_ptr<Entity>> &get_entities_by_component(int identifier);

  // gets an Entity where the given predicate returns the smallest value. Currently, this
  // method searches ALL entities, but we'll have to provide some way to limit the
  // search space (e.g. only within a certain area, etc)
  std::weak_ptr<Entity> get_entity(std::function<float(std::shared_ptr<Entity> &)> pred);

  // gets a list of all entities that match the given predicate
  std::list<std::weak_ptr<Entity>> get_entities(std::function<bool(std::shared_ptr<Entity> &)> pred);

  template<typename TComponent>
  inline std::list<std::weak_ptr<Entity>> &get_entities_by_component() {
    return get_entities_by_component(TComponent::identifier);
  }

  // gets the Entity that's currently under the cursor (if any)
  std::weak_ptr<Entity> get_entity_at_cursor();

  // gets the current center of the "view" (basically, where the camera currently sits)
  fw::Vector get_view_center() const {
    return view_center_;
  }

  void set_selection(std::weak_ptr<Entity> ent);
  void add_selection(std::weak_ptr<Entity> ent);
  void clear_selection();
  std::list<std::weak_ptr<Entity>> const &get_selection() const {
    return selected_entities_;
  }

  // gets a pointer to the EntityDebug object which contains debugging state for
  // the entities and so on.
  EntityDebug *get_debug() {
    return debug_;
  }

  void update();

  // gets the patch_manager which manages the patches of entities.
  PatchManager *get_patch_manager() const {
    return patch_mgr_;
  }
};

}

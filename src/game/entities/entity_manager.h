#pragma once

#include <memory>

#include <framework/scenegraph.h>
#include <framework/vector.h>
#include <game/entities/entity.h>

namespace fw {
class graphics;
}

namespace ent {
class entity;
class entity_debug;
class patch_manager;

/**
 * Manages all the entities in the game, and contains various "indexes" of entities so that we
 * can access them efficiently.
 */
class entity_manager {
private:
  std::list<std::shared_ptr<entity>> _all_entities;
  std::list<std::shared_ptr<entity>> _destroyed_entities;
  std::list<std::weak_ptr<entity>> _selected_entities;

  std::map<int, std::list<std::weak_ptr<entity>>> _entities_by_component;

  entity_debug *_debug;
  patch_manager *_patch_mgr;
  fw::vector _view_centre;

  // removes the destroyed entities from the various lists
  void cleanup_destroyed();

public:
  entity_manager();
  ~entity_manager();

  // initialize the entity_manager. this should be called after the
  // world has been created and the terrain is available as well..
  void initialize();

  // create a new entity based on the given .entity template
  std::shared_ptr<entity> create_entity(std::string const &template_name, entity_id id);
  std::shared_ptr<entity> create_entity(std::shared_ptr<entity> created_by, std::string const &template_name, entity_id id);

  // destroy the given entity (we actually remove it on the next update cycle)
  void destroy(std::weak_ptr<entity> entity);

  // gets the entity with the given identifier
  std::weak_ptr<entity> get_entity(entity_id id);

  // gets the entity that's closest to the given ray (used for single selection and stuff)
  std::weak_ptr<entity> get_entity(fw::vector const &start, fw::vector const &direction);

  // gets a reference to a list of all the entities with the component with the given identifier.
  std::list<std::weak_ptr<entity>> &get_entities_by_component(int identifier);

  // gets an entity where the given predicate returns the smallest value. Currently, this
  // method searches ALL entities, but we'll have to provide some way to limit the
  // search space (e.g. only within a certain area, etc)
  std::weak_ptr<entity> get_entity(std::function<float(std::shared_ptr<entity> &)> pred);

  // gets a list of all entities that match the given predicate
  std::list<std::weak_ptr<entity>> get_entities(std::function<bool(std::shared_ptr<entity> &)> pred);

  template<typename TComponent>
  inline std::list<std::weak_ptr<entity>> &get_entities_by_component() {
    return get_entities_by_component(TComponent::identifier);
  }

  // gets the entity that's currently under the cursor (if any)
  std::weak_ptr<entity> get_entity_at_cursor();

  // gets the current centre of the "view" (basically, where the camera currently sits)
  fw::vector get_view_centre() const {
    return _view_centre;
  }

  void set_selection(std::weak_ptr<entity> ent);
  void add_selection(std::weak_ptr<entity> ent);
  void clear_selection();
  std::list<std::weak_ptr<entity>> const &get_selection() const {
    return _selected_entities;
  }

  // gets a pointer to the entity_debug object which contains debugging state for
  // the entities and so on.
  entity_debug *get_debug() {
    return _debug;
  }

  void update();
  void render(fw::sg::scenegraph &scenegraph);

  // gets the patch_manager which manages the patches of entities.
  patch_manager *get_patch_manager() const {
    return _patch_mgr;
  }
};

}

#pragma once

#include <list>
#include <memory>

#include <framework/vector.h>
#include <game/entities/entity.h>

namespace ent {

// this is a "patch" for entities (with positions) exist on. The patches wrap at
// the edges of the world, just like the terrain does. By having entity patches
// that are separate to terrain patches, though, means we can make them on a different
// scale (e.g. 4 entity patches for every 1 terrain patch).
class patch {
private:
  fw::vector _origin;
  std::list<std::weak_ptr<entity>> _entities;

public:
  patch(fw::vector const &origin);
  ~patch();

  // adds the given entity to this patch
  void add_entity(std::weak_ptr<entity> entity);

  // removes the given entity from this patch
  void remove_entity(std::weak_ptr<entity> entity);

  // gets a COPY of the entity list (this could be expensive...)
  std::list<std::weak_ptr<entity>> get_entities() {
    return _entities;
  }

  fw::vector const &get_origin() const {
    return _origin;
  }
};

// this class belongs to the entity_manager and manages the "patches" which belong
// to the game itself.
class patch_manager {
public:
  static const int PATCH_SIZE = 64; // 1/2 of the terrain patch size
private:
  typedef std::vector<patch *> patch_list;
  patch_list _patches;

  int _patch_width;
  int _patch_length;

  int get_patch_index(int patch_x, int patch_z, int *new_patch_x = 0,
      int *new_patch_z = 0);

public:
  patch_manager(float size_x, float size_z);
  ~patch_manager();

  // gets the patch at the given (x, z) world-space coordinates
  patch *get_patch(float x, float z);

  // gets the patch at the given patch (x, z) coordinates.
  patch *get_patch(int patch_x, int patch_z);

  // gets the width/length of the world in "patches"
  int get_patch_width() const {
    return _patch_width;
  }
  int get_patch_length() const {
    return _patch_length;
  }

  float get_world_width() const {
    return static_cast<float>(_patch_width * PATCH_SIZE);
  }
  float get_world_length() const {
    return static_cast<float>(_patch_length * PATCH_SIZE);
  }
};

// the position component is a member of all entities that have position data (which, actually,
// is probably most of them!)
class position_component: public entity_component {
private:
  friend class entity_patch;

  fw::vector _pos;
  fw::vector _dir;
  fw::vector _up;
  bool _pos_updated;
  bool _sit_on_terrain;
  bool _orient_to_terrain;
  patch *_patch;

  // if _pos_updated is true, this will calculation "real" position of the
  // entity, taking _sit_on_terrain and _orient_to_terrain into account
  void set_final_position();

public:
  static const int identifier = 100;

  position_component();
  virtual ~position_component();

  virtual void apply_template(std::shared_ptr<entity_component_template> comp_template);

  virtual void update(float dt);

  // gets the view transform for the entity based on it's current world-space position
  fw::matrix get_transform() const;

  // gets or sets the current position of the entity
  void set_position(fw::vector const &pos);
  fw::vector get_position(bool allow_update = true);

  // gets or sets the direction the entity is facing
  void set_direction(fw::vector const &dir);
  fw::vector get_direction() const;

  // gets or sets a value which indicates whether we want to ensure the entity sits
  // on the terrain (for example, trees do, but missiles do not)
  void set_sit_on_terrain(bool sit_on_terrain);
  bool get_sit_on_terrain() const {
    return _sit_on_terrain;
  }

  // gets or sets a value which indicates whether we want to orient outselves so that
  // we're sitting flat on the terrain. This is what tanks and stuff like that will do.
  void set_orient_to_terrain(bool orient_to_terrain) {
    _orient_to_terrain = orient_to_terrain;
  }
  bool get_orient_to_terrain() const {
    return _orient_to_terrain;
  }

  // gets a pointer to the patch we're inside of
  patch *get_patch() const {
    return _patch;
  }

  // gets the direction to the given point/entity, taking into account the fact
  // that it might be quicker to wrap around the edges of the map
  fw::vector get_direction_to(fw::vector const &point) const;
  fw::vector get_direction_to(std::shared_ptr<entity> entity) const;

  // searches for the nearest entity to us which matches the given predicate
  std::weak_ptr<entity> get_nearest_entity(
      std::function<bool(std::shared_ptr<entity> const &)> pred) const;

  // searches for the nearest entity to us
  std::weak_ptr<entity> get_nearest_entity() const;

  // searches for the nearest entity, with the given component type
  std::weak_ptr<entity> get_nearest_entity_with_component(int component_type) const;

  // searches for the nearest entity, with the given component type
  template<typename T>
  inline std::weak_ptr<entity> get_nearest_entity_with_component() const {
    return get_nearest_entity_with_component(T::identifier);
  }

  // searches for all the entities within the given radius
  template<typename inserter_t>
  inline void get_entities_within_radius(float radius, inserter_t ins) const {
    std::shared_ptr<entity> us(_entity);

    std::list<std::weak_ptr<entity>> patch_entities = _patch->get_entities();
    for (auto it = patch_entities.begin(); it != patch_entities.end(); ++it) {
      std::shared_ptr<entity> ent = (*it).lock();
      if (!ent)
        continue;

      // ignore ourselves
      if (ent == us)
        continue;

      position_component *their_pos = ent->get_component<position_component>();
      if (their_pos == 0)
        continue;

      float dist = get_direction_to(their_pos->get_position()).length();
      if (dist < radius) {
        (*ins) = std::weak_ptr<entity>(ent);
      }
    }
  }

  virtual int get_identifier() {
    return identifier;
  }
};

}


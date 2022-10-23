#pragma once

#include <list>
#include <memory>

#include <framework/vector.h>
#include <game/entities/entity.h>

namespace ent {

// this is a "patch" for entities (with positions) exist on. The patches wrap at the edges of the world, just like the
// terrain does. By having Entity patches that are separate to terrain patches, though, means we can make them on a
// different scale (e.g. 4 Entity patches for every 1 terrain patch).
class Patch {
private:
  fw::Vector origin_;
  std::list<std::weak_ptr<Entity>> entities_;

public:
  Patch(fw::Vector const &origin);
  ~Patch();

  // adds the given Entity to this patch
  void add_entity(std::weak_ptr<Entity> entity);

  // removes the given Entity from this patch
  void remove_entity(std::weak_ptr<Entity> entity);

  // gets a COPY of the Entity list (this could be expensive...)
  std::list<std::weak_ptr<Entity>> get_entities() {
    return entities_;
  }

  fw::Vector const &get_origin() const {
    return origin_;
  }
};

// This class belongs to the EntityManager and manages the "patches" which belong to the game itself.
class PatchManager {
public:
  static const int PATCH_SIZE = 64; // 1/2 of the terrain patch size
private:
  std::vector<Patch*> patches_;
    int patch_width_;
  int patch_length_;

  int get_patch_index(int patch_x, int patch_z, int *new_patch_x = 0, int *new_patch_z = 0);

public:
  PatchManager(float size_x, float size_z);
  ~PatchManager();

  // gets the patch at the given (x, z) world-space coordinates
  Patch *get_patch(float x, float z);

  // gets the patch at the given patch (x, z) coordinates.
  Patch *get_patch(int patch_x, int patch_z);

  // gets the width/length of the world in "patches"
  int get_patch_width() const {
    return patch_width_;
  }
  int get_patch_length() const {
    return patch_length_;
  }

  float get_world_width() const {
    return static_cast<float>(patch_width_ * PATCH_SIZE);
  }
  float get_world_length() const {
    return static_cast<float>(patch_length_ * PATCH_SIZE);
  }
};

// the position component is a member of all entities that have position data (which, actually, is probably most of
// them!)
class PositionComponent: public EntityComponent {
private:
  fw::Vector pos_;
  fw::Vector dir_;
  fw::Vector up_;
  bool pos_updated_;
  bool sit_on_terrain_;
  bool orient_to_terrain_;
  Patch *patch_;

  // if _pos_updated is true, this will calculation "real" position of the
  // Entity, taking _sit_on_terrain and _orient_to_terrain into account
  void set_final_position();

public:
  static const int identifier = 100;

  PositionComponent();
  virtual ~PositionComponent();

  virtual void apply_template(fw::lua::Value tmpl);

  virtual void update(float dt);

  // gets the view transform for the Entity based on it's current world-space position
  fw::Matrix get_transform() const;

  // gets or sets the current position of the Entity
  void set_position(fw::Vector const &pos);
  fw::Vector get_position(bool allow_update = true);

  // gets or sets the direction the Entity is facing
  void set_direction(fw::Vector const &dir);
  fw::Vector get_direction() const;

  // gets or sets a value which indicates whether we want to ensure the Entity sits
  // on the terrain (for example, trees do, but missiles do not)
  void set_sit_on_terrain(bool sit_on_terrain);
  bool get_sit_on_terrain() const {
    return sit_on_terrain_;
  }

  // gets or sets a value which indicates whether we want to orient outselves so that
  // we're sitting flat on the terrain. This is what tanks and stuff like that will do.
  void set_orient_to_terrain(bool orient_to_terrain) {
    orient_to_terrain_ = orient_to_terrain;
  }
  bool get_orient_to_terrain() const {
    return orient_to_terrain_;
  }

  // gets a pointer to the patch we're inside of
  Patch *get_patch() const {
    return patch_;
  }

  // gets the direction to the given point/Entity, taking into account the fact
  // that it might be quicker to wrap around the edges of the map
  fw::Vector get_direction_to(fw::Vector const &point) const;
  fw::Vector get_direction_to(std::shared_ptr<Entity> Entity) const;

  // searches for the nearest Entity to us which matches the given predicate
  std::weak_ptr<Entity> get_nearest_entity(
      std::function<bool(std::shared_ptr<Entity> const &)> pred) const;

  // searches for the nearest Entity to us
  std::weak_ptr<Entity> get_nearest_entity() const;

  // searches for the nearest Entity, with the given component type
  std::weak_ptr<Entity> get_nearest_entity_with_component(int component_type) const;

  // searches for the nearest Entity, with the given component type
  template<typename T>
  inline std::weak_ptr<Entity> get_nearest_entity_with_component() const {
    return get_nearest_entity_with_component(T::identifier);
  }

  // searches for all the entities within the given radius
  template<typename inserter_t>
  inline void get_entities_within_radius(float radius, inserter_t ins) const {
    std::shared_ptr<Entity> us(entity_);

    std::list<std::weak_ptr<Entity>> patch_entities = patch_->get_entities();
    for (auto it = patch_entities.begin(); it != patch_entities.end(); ++it) {
      std::shared_ptr<Entity> ent = (*it).lock();
      if (!ent)
        continue;

      // ignore ourselves
      if (ent == us)
        continue;

      PositionComponent *their_pos = ent->get_component<PositionComponent>();
      if (their_pos == 0)
        continue;

      float dist = get_direction_to(their_pos->get_position()).length();
      if (dist < radius) {
        (*ins) = std::weak_ptr<Entity>(ent);
      }
    }
  }

  virtual int get_identifier() {
    return identifier;
  }
};

}

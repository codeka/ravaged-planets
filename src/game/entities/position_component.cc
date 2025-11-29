#include <functional>

#include <framework/misc.h>
#include <framework/scenegraph.h>
#include <framework/logging.h>

#include <game/entities/entity.h>
#include <game/entities/entity_manager.h>
#include <game/entities/entity_factory.h>
#include <game/entities/position_component.h>
#include <game/entities/mesh_component.h>
#include <game/world/world.h>
#include <game/world/terrain.h>

using namespace std::placeholders;

namespace ent {

// register the position component with the entity_factory
ENT_COMPONENT_REGISTER("Position", PositionComponent);

PositionComponent::PositionComponent() :
    pos_(0, 0, 0), dir_(0, 0, 1), up_(0, 1, 0), pos_updated_(true), sit_on_terrain_(false),
    orient_to_terrain_(false), patch_(0) {
}

PositionComponent::~PositionComponent() {
}

void PositionComponent::apply_template(fw::lua::Value tmpl) {
  set_sit_on_terrain(tmpl["SitOnTerrain"]);
  // We don't want to set this false unless it's explicitly set to false. If it's unset, then it will depend on the
  // value of SitOnTerrain.
  if (tmpl.has_key("OrientToTerrain")) {
    set_orient_to_terrain(tmpl["OrientToTerrain"]);
  }
}

void PositionComponent::set_sit_on_terrain(bool sit_on_terrain) {
  sit_on_terrain_ = sit_on_terrain;
  if (sit_on_terrain_)
    orient_to_terrain_ = true;
}

void PositionComponent::update(float) {
  set_final_position();
}

void PositionComponent::set_final_position() {
  if (pos_updated_) {
    auto terrain = game::World::get_instance()->get_terrain();
    if (sit_on_terrain_) {
      // if we're supposed to sit on the terrain, make sure we're doing that now.
      pos_ = fw::Vector(pos_[0], terrain->get_height(pos_[0], pos_[2]), pos_[2]);
    }

    if (orient_to_terrain_) {
      // if we're supposed to orient ourselves with the terrain (so it looks like we're sitting flat on the
      // terrain, rather than perfectly horizontal) do that as well. Basically, we sample the terrain at three
      // places, calculate the normal and orient ourselves to that.
      fw::Vector right = fw::cross(up_, dir_).normalized();
      fw::Vector v1 = pos_ + dir_;
      fw::Vector v2 = pos_ + right;
      fw::Vector v3 = pos_ - right;

      v1[1] = terrain->get_height(v1[0], v1[2]);
      v2[1] = terrain->get_height(v2[0], v2[2]);
      v3[1] = terrain->get_height(v3[0], v3[2]);

      up_ = fw::cross(v2 - v1, v3 - v1).normalized();
      dir_ = fw::Vector(dir_[0], 0.0f, dir_[2]).normalized();
    }

    // constrain our position to the map's dimensions
    float x = fw::constrain(pos_[0], static_cast<float>(terrain->get_width()), 0.0f);
    float z = fw::constrain(pos_[2], static_cast<float>(terrain->get_length()), 0.0f);
    pos_ = fw::Vector(x, pos_[1], z);

    // make sure we "exist" in the correct patch as well...
    std::shared_ptr<ent::Entity> entity(entity_);
    EntityManager *emgr = entity->get_manager();
    PatchManager *pmgr = emgr->get_patch_manager();
    Patch *new_patch = pmgr->get_patch(x, z);
    if (new_patch != patch_) {
      if (patch_ != nullptr) {
        patch_->remove_entity(entity);
      }

      new_patch->add_entity(entity);
      patch_ = new_patch;
    }

    pos_updated_ = false;
  }
}

void PositionComponent::set_position(fw::Vector const &pos) {
  std::shared_ptr<ent::Entity> entity(entity_);
  float world_width = entity->get_manager()->get_patch_manager()->get_world_width();
  float world_length = entity->get_manager()->get_patch_manager()->get_world_length();

  pos_ = fw::Vector(fw::constrain(pos[0], world_width, 0.0f), pos[1], fw::constrain(pos[2], world_length, 0.0f));

  pos_updated_ = true;
}

fw::Vector PositionComponent::get_position(bool allow_update) {
  if (allow_update) {
    set_final_position();
  }
  return pos_;
}

void PositionComponent::set_direction(fw::Vector const &dir) {
  dir_ = dir;
  pos_updated_ = true;
}

fw::Vector PositionComponent::get_direction() const {
  return dir_;
}

fw::Matrix PositionComponent::get_transform() const {
  fw::Matrix m = fw::identity();
  
  m *= fw::rotate(fw::Vector(0, 0, 1), dir_).to_matrix();
 // m = fw::rotate(fw::Vector(0, 1, 0), up_).to_matrix() * m;
  m *= fw::translation(pos_);

  return m;
}

fw::Vector PositionComponent::get_direction_to(fw::Vector const &point, bool ignore_height /*= false*/) const {
  fw::Vector dir = point - pos_;
  if (ignore_height) {
    dir[1] = 0.0f;
  }

  std::shared_ptr<ent::Entity> entity(entity_);
  float width = entity->get_manager()->get_patch_manager()->get_world_width();
  float length = entity->get_manager()->get_patch_manager()->get_world_length();
  for (int z = -1; z <= 1; z++) {
    for (int x = -1; x <= 1; x++) {
      fw::Vector another_point(point[0] + (x * width), point[1], point[2] + (z * length));
      fw::Vector another_dir = another_point - pos_;
      if (ignore_height) {
        another_dir[1] = 0.0f;
      }

      if (another_dir.length() < dir.length())
        dir = another_dir;
    }
  }

  return dir;
}

fw::Vector PositionComponent::get_direction_to(std::shared_ptr<Entity> entity, bool ignore_height /*= false*/) const {
  PositionComponent *their_position = entity->get_component<PositionComponent>();
  if (their_position != nullptr)
    return get_direction_to(their_position->get_position(), ignore_height);

  return fw::Vector(0, 0, 0);
}

// searches for the nearest Entity to us which matches the given predicate
std::weak_ptr<Entity> PositionComponent::get_nearest_entity(
    std::function<bool(std::shared_ptr<Entity> const &)> pred) const {
  std::shared_ptr<Entity> closest;
  float closest_distance = 0.0f;

  std::shared_ptr<Entity> us(entity_);
  std::list<std::weak_ptr<Entity>> patch_entities = patch_->get_entities();
  for (auto it = patch_entities.begin(); it != patch_entities.end(); ++it) {
    std::shared_ptr<Entity> ent = (*it).lock();
    if (!ent) {
      continue;
    }

    if (ent == us) {
      continue;
    }

    PositionComponent *their_pos = ent->get_component<PositionComponent>();
    if (their_pos == nullptr) {
      continue;
    }

    float dist = get_direction_to(their_pos->get_position()).length();
    if (!closest || (closest_distance > dist && pred(ent))) {
      closest = ent;
      closest_distance = dist;
    }
  }

  return std::weak_ptr<Entity>(closest);
}

// searches for the nearest Entity to us
std::weak_ptr<Entity> PositionComponent::get_nearest_entity() const {
  return get_nearest_entity([](std::shared_ptr<Entity> const &) { return true; });
}

std::weak_ptr<Entity> PositionComponent::get_nearest_entity_with_component(int component_type) const {
  return get_nearest_entity([component_type](std::shared_ptr<Entity> const &ent) {
    return ent->contains_component(component_type);
  });
}

//-------------------------------------------------------------------------

Patch::Patch(fw::Vector const &origin) :
    origin_(origin) {
}

Patch::~Patch() {
}

void Patch::add_entity(std::weak_ptr<Entity> Entity) {
  entities_.push_back(Entity);
}

void Patch::remove_entity(std::weak_ptr<Entity> Entity) {
  std::shared_ptr<ent::Entity> ent = Entity.lock();
  if (!ent) {
    return;
  }

  for (auto it = entities_.begin(); it != entities_.end();) {
    std::shared_ptr<ent::Entity> other_ent = (*it).lock();
    if (other_ent && other_ent == ent) {
      it = entities_.erase(it);
    } else {
      ++it;
    }
  }
}

//-------------------------------------------------------------------------

PatchManager::PatchManager(float size_x, float size_z) {
  patch_width_ = static_cast<int>(size_x / PATCH_SIZE);
  patch_length_ = static_cast<int>(size_z / PATCH_SIZE);

  patches_.resize(patch_width_ * patch_length_);
  for (int z = 0; z < patch_length_; z++) {
    for (int x = 0; x < patch_width_; x++) {
      Patch *p = new Patch(fw::Vector(static_cast<float>(x * PATCH_SIZE), 0.0f, static_cast<float>(z * PATCH_SIZE)));
      patches_[get_patch_index(x, z)] = p;
    }
  }
}

PatchManager::~PatchManager() {
  for(auto patch : patches_) {
    delete patch;
  }
  patches_.clear();
}

Patch *PatchManager::get_patch(float x, float z) {
  int patch_x = static_cast<int>(floor(x / PATCH_SIZE));
  int patch_z = static_cast<int>(floor(z / PATCH_SIZE));

  return get_patch(patch_x, patch_z);
}

Patch *PatchManager::get_patch(int patch_x, int patch_z) {
  return patches_[get_patch_index(patch_x, patch_z)];
}

int PatchManager::get_patch_index(int patch_x, int patch_z, int *new_patch_x /*= 0*/, int *new_patch_z /*= 0*/) {
  patch_x = fw::constrain(patch_x, get_patch_width());
  patch_z = fw::constrain(patch_z, get_patch_length());

  if (new_patch_x != nullptr) {
    *new_patch_x = patch_x;
  }
  if (new_patch_z != nullptr) {
    *new_patch_z = patch_z;
  }

  return (get_patch_width() * patch_z) + patch_x;
}

}

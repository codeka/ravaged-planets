#include <functional>

#include <framework/misc.h>
#include <framework/logging.h>

#include <game/entities/entity.h>
#include <game/entities/entity_manager.h>
#include <game/entities/entity_factory.h>
#include <game/entities/position_component.h>
#include <game/world/world.h>
#include <game/world/terrain.h>

using namespace std::placeholders;

namespace ent {

// register the position component with the entity_factory
ENT_COMPONENT_REGISTER("Position", position_component);

position_component::position_component() :
    pos_(0, 0, 0), dir_(0, 0, 1), _up(0, 1, 0), _pos_updated(true), _sit_on_terrain(false),
    _orient_to_terrain(false), _patch(0) {
}

position_component::~position_component() {
}

void position_component::apply_template(luabind::object const &tmpl) {
//  for (luabind::iterator it(tmpl), end; it != end; ++it) {
//    if (it.key() == "SitOnTerrain") {
//      set_sit_on_terrain(luabind::object_cast<bool>(*it));
//    } else if (it.key() == "OrientToTerrain") {
//      this->set_orient_to_terrain(luabind::object_cast<bool>(*it));
//    }
//  }
}

void position_component::set_sit_on_terrain(bool sit_on_terrain) {
  _sit_on_terrain = sit_on_terrain;
  if (_sit_on_terrain)
    _orient_to_terrain = true;
}

void position_component::update(float) {
  set_final_position();
}

void position_component::set_final_position() {
  if (_pos_updated) {
    game::terrain *terrain = game::world::get_instance()->get_terrain();
    if (_sit_on_terrain) {
      // if we're supposed to sit on the terrain, make sure we're doing that now.
      pos_ = fw::Vector(pos_[0], terrain->get_height(pos_[0], pos_[2]), pos_[2]);
    }

    if (_orient_to_terrain) {
      // if we're supposed to orient ourselves with the terrain (so it looks like we're sitting flat on the
      // terrain, rather than perfectly horizontal) do that as well. Basically, we sample the terrain at three
      // places, calculate the normal and orient ourselves to that.
      fw::Vector right = cml::cross(_up, dir_).normalize();
      fw::Vector v1 = pos_ + dir_;
      fw::Vector v2 = pos_ + right;
      fw::Vector v3 = pos_ - right;

      v1[1] = terrain->get_height(v1[0], v1[2]);
      v2[1] = terrain->get_height(v2[0], v2[2]);
      v3[1] = terrain->get_height(v3[0], v3[2]);

      _up = cml::cross(v2 - v1, v3 - v1).normalize();
      dir_ = fw::Vector(dir_[0], 0.0f, dir_[2]).normalize();
    }

    // constrain our position to the map's dimensions
    float x = fw::constrain(pos_[0], static_cast<float>(terrain->get_width()), 0.0f);
    float z = fw::constrain(pos_[2], static_cast<float>(terrain->get_length()), 0.0f);
    pos_ = fw::Vector(x, pos_[1], z);

    // make sure we "exist" in the correct patch as well...
    std::shared_ptr<ent::entity> entity(_entity);
    entity_manager *emgr = entity->get_manager();
    patch_manager *pmgr = emgr->get_patch_manager();
    patch *new_patch = pmgr->get_patch(x, z);
    if (new_patch != _patch) {
      if (_patch != nullptr) {
        _patch->remove_entity(entity);
      }

      new_patch->add_entity(entity);
      _patch = new_patch;
    }

    _pos_updated = false;
  }
}

void position_component::set_position(fw::Vector const &pos) {
  std::shared_ptr<ent::entity> entity(_entity);
  float world_width = entity->get_manager()->get_patch_manager()->get_world_width();
  float world_length = entity->get_manager()->get_patch_manager()->get_world_length();

  pos_ = fw::Vector(fw::constrain(pos[0], world_width, 0.0f), pos[1], fw::constrain(pos[2], world_length, 0.0f));

  _pos_updated = true;
}

fw::Vector position_component::get_position(bool allow_update) {
  if (allow_update) {
    set_final_position();
  }
  return pos_;
}

void position_component::set_direction(fw::Vector const &dir) {
  dir_ = dir;
  _pos_updated = true;
}

fw::Vector position_component::get_direction() const {
  return dir_;
}

fw::Matrix position_component::get_transform() const {
  fw::Matrix m = fw::identity();
  m *= fw::rotate(fw::Vector(0, 0, 1), dir_);
  m *= fw::rotate(fw::Vector(0, 1, 0), _up);
  m *= fw::translation(pos_);

  return m;
}

fw::Vector position_component::get_direction_to(fw::Vector const &point) const {
  fw::Vector dir = point - pos_;

  std::shared_ptr<ent::entity> entity(_entity);
  float width = entity->get_manager()->get_patch_manager()->get_world_width();
  float length = entity->get_manager()->get_patch_manager()->get_world_length();
  for (int z = -1; z <= 1; z++) {
    for (int x = -1; x <= 1; x++) {
      fw::Vector another_point(point[0] + (x * width), point[1], point[2] + (z * length));
      fw::Vector another_dir = another_point - pos_;

      if (another_dir.length_squared() < dir.length_squared())
        dir = another_dir;
    }
  }

  return dir;
}

fw::Vector position_component::get_direction_to(std::shared_ptr<entity> entity) const {
  position_component *their_position = entity->get_component<position_component>();
  if (their_position != nullptr)
    return get_direction_to(their_position->get_position());

  return fw::Vector(0, 0, 0);
}

// searches for the nearest entity to us which matches the given predicate
std::weak_ptr<entity> position_component::get_nearest_entity(
    std::function<bool(std::shared_ptr<entity> const &)> pred) const {
  std::shared_ptr<entity> closest;
  float closest_distance = 0.0f;

  std::shared_ptr<entity> us(_entity);
  std::list<std::weak_ptr<entity>> patch_entities = _patch->get_entities();
  for (auto it = patch_entities.begin(); it != patch_entities.end(); ++it) {
    std::shared_ptr<entity> ent = (*it).lock();
    if (!ent) {
      continue;
    }

    if (ent == us) {
      continue;
    }

    position_component *their_pos = ent->get_component<position_component>();
    if (their_pos == nullptr) {
      continue;
    }

    float dist = get_direction_to(their_pos->get_position()).length_squared();
    if (!closest || (closest_distance > dist && pred(ent))) {
      closest = ent;
      closest_distance = dist;
    }
  }

  return std::weak_ptr<entity>(closest);
}

// searches for the nearest entity to us
std::weak_ptr<entity> position_component::get_nearest_entity() const {
  return get_nearest_entity([](std::shared_ptr<entity> const &) { return true; });
}

std::weak_ptr<entity> position_component::get_nearest_entity_with_component(int component_type) const {
  return get_nearest_entity([component_type](std::shared_ptr<entity> const &ent) {
    return ent->contains_component(component_type);
  });
}

//-------------------------------------------------------------------------

patch::patch(fw::Vector const &origin) :
    _origin(origin) {
}

patch::~patch() {
}

void patch::add_entity(std::weak_ptr<entity> entity) {
  _entities.push_back(entity);
}

void patch::remove_entity(std::weak_ptr<entity> entity) {
  std::shared_ptr<ent::entity> ent = entity.lock();
  if (!ent) {
    return;
  }

  for (auto it = _entities.begin(); it != _entities.end();) {
    std::shared_ptr<ent::entity> other_ent = (*it).lock();
    if (other_ent && other_ent == ent) {
      it = _entities.erase(it);
    } else {
      ++it;
    }
  }
}

//-------------------------------------------------------------------------

patch_manager::patch_manager(float size_x, float size_z) {
  _patch_width = static_cast<int>(size_x / PATCH_SIZE);
  _patch_length = static_cast<int>(size_z / PATCH_SIZE);

  _patches.resize(_patch_width * _patch_length);
  for (int z = 0; z < _patch_length; z++) {
    for (int x = 0; x < _patch_width; x++) {
      patch *p = new patch(fw::Vector(static_cast<float>(x * PATCH_SIZE), 0.0f, static_cast<float>(z * PATCH_SIZE)));
      _patches[get_patch_index(x, z)] = p;
    }
  }
}

patch_manager::~patch_manager() {
  for(patch_list::value_type patch : _patches) {
    delete patch;
  }
  _patches.clear();
}

patch *patch_manager::get_patch(float x, float z) {
  int patch_x = static_cast<int>(floor(x / PATCH_SIZE));
  int patch_z = static_cast<int>(floor(z / PATCH_SIZE));

  return get_patch(patch_x, patch_z);
}

patch *patch_manager::get_patch(int patch_x, int patch_z) {
  return _patches[get_patch_index(patch_x, patch_z)];
}

int patch_manager::get_patch_index(int patch_x, int patch_z, int *new_patch_x /*= 0*/, int *new_patch_z /*= 0*/) {
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

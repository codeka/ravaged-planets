#include <functional>

#include <framework/framework.h>
#include <framework/camera.h>
#include <framework/exception.h>
#include <framework/input.h>
#include <framework/graphics.h>
#include <framework/timer.h>
#include <framework/misc.h>
#include <framework/logging.h>

#include <game/world/terrain.h>
#include <game/world/world.h>
#include <game/entities/entity.h>
#include <game/entities/entity_factory.h>
#include <game/entities/entity_manager.h>
#include <game/entities/entity_debug.h>
#include <game/entities/position_component.h>
#include <game/entities/ownable_component.h>
#include <game/entities/selectable_component.h>

using namespace std::placeholders;

namespace ent {

EntityManager::EntityManager() :
    patch_mgr_(0), debug_(0) {
}

EntityManager::~EntityManager() {
  delete patch_mgr_;
  delete debug_;
}

void EntityManager::initialize() {
  game::World *wrld = game::World::get_instance();
  game::Terrain *trn = wrld->get_terrain();

  debug_ = new EntityDebug(this);
  patch_mgr_ = new PatchManager(static_cast<float>(trn->get_width()), static_cast<float>(trn->get_length()));
}

std::shared_ptr<Entity> EntityManager::create_entity(std::string const &template_name, entity_id id) {
  return create_entity(std::shared_ptr<Entity>(), template_name, id);
}

std::shared_ptr<Entity> EntityManager::create_entity(
    std::shared_ptr<Entity> created_by, std::string const &template_name, entity_id id) {
  std::shared_ptr<Entity> ent(new Entity(this, id));
  ent->name_ = template_name;
  ent->creator_ = created_by;

  ent::EntityFactory factory;
  factory.populate(ent, template_name);

  ent->initialize();
  if (created_by) {
    // if we're being created by another Entity, some of our properties get copied to the new Entity (particularly
    // debug flags)
    ent->set_debug_flags(created_by->get_debug_flags());

    // it'll also start at the same position as the creator
    PositionComponent *creator_pos = created_by->get_component<PositionComponent>();
    PositionComponent *new_pos = ent->get_component<PositionComponent>();
    if (new_pos != nullptr && creator_pos != nullptr) {
      new_pos->set_position(creator_pos->get_position());
    }

    // if they're both ownable, set the new Entity's owner to the given value.
    OwnableComponent *creator_ownable = created_by->get_component<OwnableComponent>();
    OwnableComponent *new_ownable = ent->get_component<OwnableComponent>();
    if (new_ownable != nullptr && creator_ownable != nullptr) {
      new_ownable->set_owner(creator_ownable->get_owner());
    }
  }

  ent->add_attribute(ent::EntityAttribute("patch_offset_", fw::Vector(0, 0, 0)));

  fw::debug << boost::format("created entity: %1% (identifier: %2%)") % template_name % id << std::endl;

  for (auto& pair : ent->components_) {
    EntityComponent *comp = pair.second;
    if (comp->allow_get_by_component()) {
      std::list<std::weak_ptr<Entity>> &entities_by_component = get_entities_by_component(comp->get_identifier());
      entities_by_component.push_back(ent);
    }
  }

  all_entities_.push_back(ent);
  return ent;
}

void EntityManager::destroy(std::weak_ptr<Entity> entity) {
  std::shared_ptr<ent::Entity> sp = entity.lock();
  if (sp) {
    float age = sp->get_age();
    fw::debug << boost::format("destroying entity: %1% (age: %2%)") % sp->get_name() % age << std::endl;

    destroyed_entities_.push_back(sp);
  }
}

// gets an Entity where the given predicate returns the smallest value. Currently, this
// method searches ALL entities, but we'll have to provide some way to limit the
// search space (e.g. only within a certain area, etc)
std::weak_ptr<Entity> EntityManager::get_entity(std::function<float(std::shared_ptr<Entity> &)> pred) {
  std::shared_ptr<Entity> curr_entity;
  float last_pred = 0.0f;
  for(std::shared_ptr<Entity> &ent : all_entities_) {
    if (!curr_entity) {
      curr_entity = ent;
      last_pred = pred(ent);
    } else {
      float this_pred = pred(ent);
      if (this_pred < last_pred) {
        curr_entity = ent;
        last_pred = this_pred;
      }
    }
  }

  return std::weak_ptr<Entity> (curr_entity);
}

std::list<std::weak_ptr<Entity>> EntityManager::get_entities(std::function<bool(std::shared_ptr<Entity> &)> pred) {
  std::list<std::weak_ptr<Entity>> entities;
  for (std::shared_ptr<Entity>& ent : all_entities_) {
    if (pred(ent)) {
      entities.push_back(std::weak_ptr<Entity > (ent));
    }
  }

  return entities;
}

std::weak_ptr<Entity> EntityManager::get_entity_at_cursor() {
  fw::Framework *frmwrk = fw::Framework::get_instance();
  fw::Input *Input = frmwrk->get_input();
  float mx = (float) Input->mouse_x();
  float my = (float) Input->mouse_y();

  mx = 1.0f - (2.0f * mx / frmwrk->get_graphics()->get_width());
  my = 1.0f - (2.0f * my / frmwrk->get_graphics()->get_height());

  fw::Camera *camera = frmwrk->get_camera();
  fw::Vector mvec = camera->unproject(-mx, my);

  fw::Vector start = camera->get_position();
  fw::Vector direction = (mvec - start).normalize();

  return get_entity(start, direction);
}

std::weak_ptr<Entity> EntityManager::get_entity(fw::Vector const &start, fw::Vector const &direction) {
  fw::Vector location = get_view_center();

  int centre_patch_x = (int) (location[0] / PatchManager::PATCH_SIZE);
  int centre_patch_z = (int) (location[2] / PatchManager::PATCH_SIZE);

  for (int patch_z = centre_patch_z - 1; patch_z <= centre_patch_z + 1; patch_z++) {
    for (int patch_x = centre_patch_x - 1; patch_x <= centre_patch_x + 1; patch_x++) {
      Patch *p = patch_mgr_->get_patch(patch_x, patch_z);

      fw::Vector offset = fw::Vector(
          (float) (patch_x * PatchManager::PATCH_SIZE) - p->get_origin()[0],
          0,
          (float) (patch_z * PatchManager::PATCH_SIZE) - p->get_origin()[2]);

      std::list<std::weak_ptr<Entity> > patch_entities = p->get_entities();
      for (auto it = patch_entities.begin(); it != patch_entities.end(); ++it) {
        std::shared_ptr<Entity> Entity = (*it).lock();
        if (!Entity)
          continue;

        SelectableComponent *sel = Entity->get_component<SelectableComponent>();
        if (sel == nullptr)
          continue;

        PositionComponent *pos = Entity->get_component<PositionComponent>();
        if (pos == nullptr)
          continue;

        float distance = fw::distance_between_line_and_point(start, direction, pos->get_position(false) + offset);
        if (distance < sel->get_selection_radius()) {
          return std::weak_ptr < ent::Entity > (Entity);
        }
      }
    }
  }

  return std::weak_ptr<Entity>();
}

std::weak_ptr<Entity> EntityManager::get_entity(entity_id id) {
  // obviously, this is dumb... we should index entities by the identifier for fast access
  for (std::shared_ptr<Entity>& ent : all_entities_) {
    if (ent->get_id() == id)
      return std::weak_ptr<Entity>(ent);
  }

  return std::weak_ptr<Entity>();
}

// gets a reference to a list of all the entities with the component with the given identifier.
std::list<std::weak_ptr<Entity> > &EntityManager::get_entities_by_component(int identifier) {
  auto it = entities_by_component_.find(identifier);
  if (it == entities_by_component_.end()) {
    // put a new one on and return that
    entities_by_component_[identifier] = std::list<std::weak_ptr<Entity>>();
    it = entities_by_component_.find(identifier);
  }

  return it->second;
}

void EntityManager::set_selection(std::weak_ptr<Entity> ent) {
  clear_selection();
  add_selection(ent);
}

void EntityManager::add_selection(std::weak_ptr<Entity> ent) {
  std::shared_ptr<Entity> sp = ent.lock();
  if (sp) {
    SelectableComponent *sel = sp->get_component<SelectableComponent>();
    if (sel != 0) {
      sel->set_is_selected(true);
      selected_entities_.push_back(ent);
    }
  }
}

void EntityManager::clear_selection() {
  // make sure all the currently-selected components know they're no longer selected
  for(auto const &sel_entity : selected_entities_) {
    std::shared_ptr<Entity> ent = sel_entity.lock();
    if (!ent) {
      continue;
    }

    SelectableComponent *sel = ent->get_component<SelectableComponent>();
    if (sel == nullptr) {
      continue; // it shouldn't be here if that's the case, but who knows?
    }

    sel->set_is_selected(false);
  }

  selected_entities_.clear();
}

void EntityManager::cleanup_destroyed() {
  // go through the destroyed list and destroy all entities that have been marked as such
  for(auto ent : destroyed_entities_) {
    for (auto it = all_entities_.begin(); it != all_entities_.end();) {
      if (*it == ent) {
        it = all_entities_.erase(it);
      } else {
        ++it;
      }
    }
  }
  destroyed_entities_.clear();

  // clear the other Entity list(s) of entities that have been destroyed
  selected_entities_.remove_if(std::bind(&std::weak_ptr<Entity> ::expired, _1));

  for(auto it : entities_by_component_) {
    it.second.remove_if(std::bind(&std::weak_ptr<Entity>::expired, _1));
  }
}

void EntityManager::update() {
  cleanup_destroyed();

  // work out the current "view center" which is used for things like drawing
  // the entities centred around the camera and so on.
  game::World *wrld = game::World::get_instance();
  fw::Camera *camera = fw::Framework::get_instance()->get_camera();
  fw::Vector cam_loc = camera->get_position();
  fw::Vector cam_dir = camera->get_direction();

  fw::Vector location = wrld->get_terrain()->get_cursor_location(cam_loc, cam_dir);
  view_center_ = fw::Vector(
      fw::constrain(location[0], this->get_patch_manager()->get_world_width(), 0.0f),
      location[1],
      fw::constrain(location[2], this->get_patch_manager()->get_world_length(), 0.0f));

  // update all of the entities
  float dt = fw::Framework::get_instance()->get_timer()->get_update_time();
  for(auto &ent : all_entities_) {
    ent->update(dt);
  }

  int center_patch_x = (int)(location[0] / PatchManager::PATCH_SIZE);
  int center_patch_z = (int)(location[2] / PatchManager::PATCH_SIZE);
  for (int patch_z = center_patch_z - 1; patch_z <= center_patch_z + 1; patch_z++) {
    for (int patch_x = center_patch_x - 1; patch_x <= center_patch_x + 1; patch_x++) {
      Patch* p = patch_mgr_->get_patch(patch_x, patch_z);

      fw::Vector patch_offset(
        (float)(patch_x * PatchManager::PATCH_SIZE) - p->get_origin()[0],
        0,
        (float)(patch_z * PatchManager::PATCH_SIZE) - p->get_origin()[2]);

      std::list<std::weak_ptr<Entity> > patch_entities = p->get_entities();
      for (auto it = patch_entities.begin(); it != patch_entities.end(); ++it) {
        auto entity = (*it).lock();
        if (!entity)
          continue;

        auto attr = entity->get_attribute("patch_offset_");
        if (attr != nullptr) {
          attr->set_value(patch_offset);
        }
      }
    }
  }


  // update the EntityDebug interface
  debug_->update();
}

}

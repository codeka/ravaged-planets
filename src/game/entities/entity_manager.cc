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

entity_manager::entity_manager() :
    _patch_mgr(0), _debug(0) {
}

entity_manager::~entity_manager() {
  delete _patch_mgr;
  delete _debug;
}

void entity_manager::initialize() {
  game::world *wrld = game::world::get_instance();
  game::terrain *trn = wrld->get_terrain();

  _debug = new entity_debug(this);
  _patch_mgr = new patch_manager(static_cast<float>(trn->get_width()), static_cast<float>(trn->get_length()));
}

std::shared_ptr<entity> entity_manager::create_entity(std::string const &template_name, entity_id id) {
  return create_entity(std::shared_ptr<entity>(), template_name, id);
}

std::shared_ptr<entity> entity_manager::create_entity(std::shared_ptr<entity> created_by,
    std::string const &template_name, entity_id id) {
  std::shared_ptr<entity> ent(new entity(this, id));
  ent->_name = template_name;
  ent->_creator = created_by;

  ent::entity_factory factory;
  factory.populate(ent, template_name);

  ent->initialize();
  if (created_by) {
    // if we're being created by another entity, some of our properties
    // get copied to the new entity (particularly debug flags)
    ent->set_debug_flags(created_by->get_debug_flags());

    // it'll also start at the same position as the creator
    position_component *creator_pos = created_by->get_component<position_component>();
    position_component *new_pos = ent->get_component<position_component>();
    if (new_pos != nullptr && creator_pos != nullptr) {
      new_pos->set_position(creator_pos->get_position());
    }

    // if they're both ownable, set the new entity's owner to the given value.
    ownable_component *creator_ownable = created_by->get_component<ownable_component>();
    ownable_component *new_ownable = ent->get_component<ownable_component>();
    if (new_ownable != nullptr && creator_ownable != nullptr) {
      new_ownable->set_owner(creator_ownable->get_owner());
    }
  }

  fw::debug << boost::format("created entity: %1% (identifier: %2%)") % template_name % id << std::endl;

  for (auto& pair : ent->_components) {
    entity_component *comp = pair.second;
    if (comp->allow_get_by_component()) {
      std::list<std::weak_ptr<entity>> &entities_by_component = get_entities_by_component(comp->get_identifier());
      entities_by_component.push_back(ent);
    }
  }

  _all_entities.push_back(ent);
  return ent;
}

void entity_manager::destroy(std::weak_ptr<entity> entity) {
  std::shared_ptr<ent::entity> sp = entity.lock();
  if (sp) {
    float age = sp->get_age();
    fw::debug << boost::format("destroying entity: %1% (age: %2%)") % sp->get_name() % age << std::endl;

    _destroyed_entities.push_back(sp);
  }
}

// gets an entity where the given predicate returns the smallest value. Currently, this
// method searches ALL entities, but we'll have to provide some way to limit the
// search space (e.g. only within a certain area, etc)
std::weak_ptr<entity> entity_manager::get_entity(std::function<float(std::shared_ptr<entity> &)> pred) {
  std::shared_ptr<entity> curr_entity;
  float last_pred = 0.0f;
  for(std::shared_ptr<entity> &ent : _all_entities) {
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

  return std::weak_ptr<entity> (curr_entity);
}

std::list<std::weak_ptr<entity>> entity_manager::get_entities(std::function<bool(std::shared_ptr<entity> &)> pred) {
  std::list<std::weak_ptr<entity>> entities;
  for (std::shared_ptr<entity>& ent : _all_entities) {
    if (pred(ent)) {
      entities.push_back(std::weak_ptr<entity > (ent));
    }
  }

  return entities;
}

std::weak_ptr<entity> entity_manager::get_entity_at_cursor() {
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

std::weak_ptr<entity> entity_manager::get_entity(fw::Vector const &start, fw::Vector const &direction) {
  fw::Vector location = get_view_centre();

  int centre_patch_x = (int) (location[0] / patch_manager::PATCH_SIZE);
  int centre_patch_z = (int) (location[2] / patch_manager::PATCH_SIZE);

  for (int patch_z = centre_patch_z - 1; patch_z <= centre_patch_z + 1; patch_z++) {
    for (int patch_x = centre_patch_x - 1; patch_x <= centre_patch_x + 1; patch_x++) {
      patch *p = _patch_mgr->get_patch(patch_x, patch_z);

      fw::Vector offset = fw::Vector(
          (float) (patch_x * patch_manager::PATCH_SIZE) - p->get_origin()[0],
          0,
          (float) (patch_z * patch_manager::PATCH_SIZE) - p->get_origin()[2]);

      std::list<std::weak_ptr<entity> > patch_entities = p->get_entities();
      for (auto it = patch_entities.begin(); it != patch_entities.end(); ++it) {
        std::shared_ptr<entity> entity = (*it).lock();
        if (!entity)
          continue;

        selectable_component *sel = entity->get_component<selectable_component>();
        if (sel == nullptr)
          continue;

        position_component *pos = entity->get_component<position_component>();
        if (pos == nullptr)
          continue;

        float distance = fw::distance_between_line_and_point(start, direction, pos->get_position(false) + offset);
        if (distance < sel->get_selection_radius()) {
          return std::weak_ptr < ent::entity > (entity);
        }
      }
    }
  }

  return std::weak_ptr<entity>();
}

std::weak_ptr<entity> entity_manager::get_entity(entity_id id) {
  // obviously, this is dumb... we should index entities by the identifier for fast access
  for (std::shared_ptr<entity>& ent : _all_entities) {
    if (ent->get_id() == id)
      return std::weak_ptr<entity>(ent);
  }

  return std::weak_ptr<entity>();
}

// gets a reference to a list of all the entities with the component with the given identifier.
std::list<std::weak_ptr<entity> > &entity_manager::get_entities_by_component(int identifier) {
  auto it = _entities_by_component.find(identifier);
  if (it == _entities_by_component.end()) {
    // put a new one on and return that
    _entities_by_component[identifier] = std::list<std::weak_ptr<entity>>();
    it = _entities_by_component.find(identifier);
  }

  return it->second;
}

void entity_manager::set_selection(std::weak_ptr<entity> ent) {
  clear_selection();
  add_selection(ent);
}

void entity_manager::add_selection(std::weak_ptr<entity> ent) {
  std::shared_ptr<entity> sp = ent.lock();
  if (sp) {
    selectable_component *sel = sp->get_component<selectable_component>();
    if (sel != 0) {
      sel->set_is_selected(true);
      _selected_entities.push_back(ent);
    }
  }
}

void entity_manager::clear_selection() {
  // make sure all the currently-selected components know they're no longer selected
  for(auto const &sel_entity : _selected_entities) {
    std::shared_ptr<entity> ent = sel_entity.lock();
    if (!ent) {
      continue;
    }

    selectable_component *sel = ent->get_component<selectable_component>();
    if (sel == nullptr) {
      continue; // it shouldn't be here if that's the case, but who knows?
    }

    sel->set_is_selected(false);
  }

  _selected_entities.clear();
}

void entity_manager::cleanup_destroyed() {
  // go through the destroyed list and destroy all entities that have been marked as such
  for(auto ent : _destroyed_entities) {
    for (auto it = _all_entities.begin(); it != _all_entities.end();) {
      if (*it == ent) {
        it = _all_entities.erase(it);
      } else {
        ++it;
      }
    }
  }
  _destroyed_entities.clear();

  // clear the other entity list(s) of entities that have been destroyed
  _selected_entities.remove_if(std::bind(&std::weak_ptr<entity> ::expired, _1));

  for(auto it : _entities_by_component) {
    it.second.remove_if(std::bind(&std::weak_ptr<entity>::expired, _1));
  }
}

void entity_manager::update() {
  cleanup_destroyed();

  // work out the current "view center" which is used for things like drawing
  // the entities centred around the camera and so on.
  game::world *wrld = game::world::get_instance();
  fw::Camera *camera = fw::Framework::get_instance()->get_camera();
  fw::Vector cam_loc = camera->get_position();
  fw::Vector cam_dir = camera->get_direction();

  fw::Vector location = wrld->get_terrain()->get_cursor_location(cam_loc, cam_dir);
  _view_centre = fw::Vector(
      fw::constrain(location[0], this->get_patch_manager()->get_world_width(), 0.0f),
      location[1],
      fw::constrain(location[2], this->get_patch_manager()->get_world_length(), 0.0f));

  // update all of the entities
  float dt = fw::Framework::get_instance()->get_timer()->get_frame_time();
  for(auto &ent : _all_entities) {
    ent->update(dt);
  }

  // update the entity_debug interface
  _debug->update();
}

void entity_manager::render(fw::sg::Scenegraph &Scenegraph) {
  fw::Vector location = get_view_centre();

  int centre_patch_x = (int) (location[0] / patch_manager::PATCH_SIZE);
  int centre_patch_z = (int) (location[2] / patch_manager::PATCH_SIZE);

  for (int patch_z = centre_patch_z - 1; patch_z <= centre_patch_z + 1; patch_z++) {
    for (int patch_x = centre_patch_x - 1; patch_x <= centre_patch_x + 1; patch_x++) {
      patch *p = _patch_mgr->get_patch(patch_x, patch_z);

      fw::Matrix trans = fw::translation(fw::Vector(
          (float) (patch_x * patch_manager::PATCH_SIZE) - p->get_origin()[0],
          0,
          (float) (patch_z * patch_manager::PATCH_SIZE) - p->get_origin()[2]));

      std::list<std::weak_ptr<entity> > patch_entities = p->get_entities();
      for (auto it = patch_entities.begin(); it != patch_entities.end(); ++it) {
        std::shared_ptr<entity> entity = (*it).lock();
        if (!entity)
          continue;

        entity->render(Scenegraph, trans);
      }
    }
  }
}

}

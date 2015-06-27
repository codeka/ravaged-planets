//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//

#include "stdafx.h"
#include "entity.h"
#include "entity_factory.h"
#include "entity_manager.h"
#include "entity_debug.h"
#include "position_component.h"
#include "ownable_component.h"
#include "selectable_component.h"
#include "../world/terrain.h"
#include "../world/world.h"
#include "../../framework/framework.h"
#include "../../framework/camera.h"
#include "../../framework/exception.h"
#include "../../framework/input.h"
#include "../../framework/main_window.h"
#include "../../framework/timer.h"
#include "../../framework/misc.h"
#include "../../framework/logging.h"

namespace ent {

	entity_manager::entity_manager()
		: _patch_mgr(0), _debug(0)
	{
	}

	entity_manager::~entity_manager()
	{
		delete _patch_mgr;
		delete _debug;
	}

	void entity_manager::initialise()
	{
		ww::world *wrld = ww::world::get_instance();
		ww::terrain *trn = wrld->get_terrain();

		_debug = new entity_debug(this);
		_patch_mgr = new patch_manager(
			static_cast<float>(trn->get_width()), static_cast<float>(trn->get_length()));
	}

	shared_ptr<entity> entity_manager::create_entity(std::string const &template_name, entity_id id)
	{
		return create_entity(shared_ptr<entity>(), template_name, id);
	}

	shared_ptr<entity> entity_manager::create_entity(shared_ptr<entity> created_by, std::string const &template_name, entity_id id)
	{
		shared_ptr<entity> ent(new entity(this, id));
		ent->_name = template_name;
		ent->_creator = created_by;

		ent::entity_factory factory;
		factory.populate(ent, template_name);

		ent->initialise();
		if (created_by)
		{
			// if we're being created by another entity, some of our properties
			// get copied to the new entity (particularly debug flags)
			ent->set_debug_flags(created_by->get_debug_flags());

			// it'll also start at the same position as the creator
			position_component *creator_pos = created_by->get_component<position_component>();
			position_component *new_pos = ent->get_component<position_component>();
			if (new_pos != 0 && creator_pos != 0)
			{
				new_pos->set_position(creator_pos->get_position());
			}

			// if they're both ownable, set the new entity's owner to the given value.
			ownable_component *creator_ownable = created_by->get_component<ownable_component>();
			ownable_component *new_ownable = ent->get_component<ownable_component>();
			if (new_ownable != 0 && creator_ownable != 0)
			{
				new_ownable->set_owner(creator_ownable->get_owner());
			}
		}

		fw::debug << boost::format("created entity: %1% (identifier: %2%)")
			% template_name % id << std::endl;

		BOOST_FOREACH(entity::component_map::value_type it, ent->_components)
		{
			entity_component *comp = it.second;
			if (comp->allow_get_by_component())
			{
				std::list< weak_ptr<entity> > &entities_by_component = get_entities_by_component(comp->get_identifier());
				entities_by_component.push_back(ent);
			}
		}

		_all_entities.push_back(ent);
		return ent;
	}

	void entity_manager::destroy(weak_ptr<entity> entity)
	{
		shared_ptr<ent::entity> sp = entity.lock();
		if (sp)
		{
			float age = sp->get_age();
			fw::debug << boost::format("destroying entity: %1% (age: %2%)")
				% sp->get_name() % age << std::endl;

			_destroyed_entities.push_back(sp);
		}
	}

	// gets an entity where the given predicate returns the smallest value. Currently, this
	// method searches ALL entities, but we'll have to provide some way to limit the
	// search space (e.g. only within a certain area, etc)
	weak_ptr<entity> entity_manager::get_entity(boost::function<float (shared_ptr<entity> &)> pred)
	{
		shared_ptr<entity> curr_entity;
		float last_pred = 0.0f;
		BOOST_FOREACH(shared_ptr<entity> &ent, _all_entities)
		{
			if (!curr_entity)
			{
				curr_entity = ent;
				last_pred = pred(ent);
			}
			else
			{
				float this_pred = pred(ent);
				if (this_pred < last_pred)
				{
					curr_entity = ent;
					last_pred = this_pred;
				}
			}
		}

		return weak_ptr<entity>(curr_entity);
	}

	std::list< weak_ptr<entity> > entity_manager::get_entities(boost::function<bool (shared_ptr<entity> &)> pred)
	{
		std::list< weak_ptr<entity> > entities;
		BOOST_FOREACH(shared_ptr<entity> &ent, _all_entities)
		{
			if (pred(ent))
			{
				entities.push_back(weak_ptr<entity>(ent));
			}
		}

		return entities;
	}

	weak_ptr<entity> entity_manager::get_entity_at_cursor()
	{
		fw::framework *frmwrk = fw::framework::get_instance();
		fw::input *input = frmwrk->get_input();
		float mx = (float) input->mouse_x();
		float my = (float) input->mouse_y();

		mx = 1.0f - (2.0f * mx / frmwrk->get_window()->get_width());
		my = 1.0f - (2.0f * my / frmwrk->get_window()->get_height());

		fw::camera *camera = frmwrk->get_camera();
		fw::vector mvec = camera->unproject(-mx, my);

		fw::vector start = camera->get_position();
		fw::vector direction = (mvec - start).normalize();

		return get_entity(start, direction);
	}

	weak_ptr<entity> entity_manager::get_entity(fw::vector const &start, fw::vector const &direction)
	{
		fw::vector location = get_view_centre();

		int centre_patch_x = (int)(location[0] / patch_manager::PATCH_SIZE);
		int centre_patch_z = (int)(location[2] / patch_manager::PATCH_SIZE);

		for(int patch_z = centre_patch_z - 1; patch_z <= centre_patch_z + 1; patch_z++)
		{
			for(int patch_x = centre_patch_x - 1; patch_x <= centre_patch_x + 1; patch_x++)
			{
				patch *p = _patch_mgr->get_patch(patch_x, patch_z);

				fw::vector offset = fw::vector(
					(float) (patch_x * patch_manager::PATCH_SIZE) - p->get_origin()[0],
					0,
					(float) (patch_z * patch_manager::PATCH_SIZE) - p->get_origin()[2]);

				std::list< weak_ptr<entity> > patch_entities = p->get_entities();
				for(std::list< weak_ptr<entity> >::iterator it = patch_entities.begin();
					it != patch_entities.end(); ++it)
				{
					shared_ptr<entity> entity = (*it).lock();
					if (!entity)
						continue;

					selectable_component *sel = entity->get_component<selectable_component>();
					if (sel == 0)
						continue;

					position_component *pos = entity->get_component<position_component>();
					if (pos == 0)
						continue;

					float distance = fw::distance_between_line_and_point(start, direction, pos->get_position(false) + offset);
					if (distance < sel->get_selection_radius())
					{
						return weak_ptr<ent::entity>(entity);
					}
				}
			}
		}

		return weak_ptr<entity>();
	}

	weak_ptr<entity> entity_manager::get_entity(entity_id id)
	{
		// obviously, this is dumb... we should index entities by the identifier for fast access
		BOOST_FOREACH(shared_ptr<entity> &ent, _all_entities)
		{
			if (ent->get_id() == id)
				return weak_ptr<entity>(ent);
		}

		return weak_ptr<entity>();
	}

	// gets a reference to a list of all the entities with the component with the given identifier.
	std::list< weak_ptr<entity> > &entity_manager::get_entities_by_component(int identifier)
	{
		entities_by_component_map::iterator it = _entities_by_component.find(identifier);
		if (it == _entities_by_component.end())
		{
			// put a new one on and return that
			_entities_by_component[identifier] = std::list< weak_ptr<entity> >();
			it = _entities_by_component.find(identifier);
		}

		return it->second;
	}

	void entity_manager::set_selection(weak_ptr<entity> ent)
	{
		clear_selection();
		add_selection(ent);
	}

	void entity_manager::add_selection(weak_ptr<entity> ent)
	{
		shared_ptr<entity> sp = ent.lock();
		if (sp)
		{
			selectable_component *sel = sp->get_component<selectable_component>();
			if (sel != 0)
			{
				sel->set_is_selected(true);
				_selected_entities.push_back(ent);
			}
		}
	}

	void entity_manager::clear_selection()
	{
		// make sure all the currently-selected components know they're no longer selected
		BOOST_FOREACH(weak_ptr<entity> const &sel_entity, _selected_entities)
		{
			shared_ptr<entity> ent = sel_entity.lock();
			if (!ent) continue;

			selectable_component *sel = ent->get_component<selectable_component>();
			if (sel == 0)
				continue; // it shouldn't be here if that's the case, but who knows?

			sel->set_is_selected(false);
		}

		_selected_entities.clear();
	}

	void entity_manager::cleanup_destroyed()
	{
		// go through the destroyed list and destroy all entities that have been marked as such
		BOOST_FOREACH(shared_ptr<entity> ent, _destroyed_entities)
		{
			for(std::list< shared_ptr<entity> >::iterator it = _all_entities.begin();
				it != _all_entities.end();)
			{
				if (*it == ent)
				{
					it = _all_entities.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
		_destroyed_entities.clear();

		// clear the other entity list(s) of entities that have been destroyed
		_selected_entities.remove_if(boost::bind(&weak_ptr<entity>::expired, _1));

		BOOST_FOREACH(entities_by_component_map::value_type it, _entities_by_component)
		{
			it.second.remove_if(boost::bind(&weak_ptr<entity>::expired, _1));
		}
	}

	void entity_manager::update()
	{
		cleanup_destroyed();

		// work out the current "view centre" which is used for things like drawing
		// the entities centred around the camera and so on.
		ww::world *wrld = ww::world::get_instance();
		fw::camera *camera = fw::framework::get_instance()->get_camera();
		fw::vector cam_loc = camera->get_position();
		fw::vector cam_dir = camera->get_direction();

		fw::vector location = wrld->get_terrain()->get_cursor_location(cam_loc, cam_dir);
		_view_centre  = fw::vector(
			fw::constrain(location[0], this->get_patch_manager()->get_world_width(), 0.0f),
			location[1],
			fw::constrain(location[2], this->get_patch_manager()->get_world_length(), 0.0f));

		// update all of the entities
		float dt = fw::framework::get_instance()->get_timer()->get_frame_time();
		BOOST_FOREACH(shared_ptr<entity> &ent, _all_entities)
		{
			ent->update(dt);
		}

		// update the entity_debug interface
		_debug->update();
	}

	void entity_manager::render(fw::sg::scenegraph &scenegraph)
	{
		fw::vector location = get_view_centre();

		int centre_patch_x = (int)(location[0] / patch_manager::PATCH_SIZE);
		int centre_patch_z = (int)(location[2] / patch_manager::PATCH_SIZE);

		for(int patch_z = centre_patch_z - 1; patch_z <= centre_patch_z + 1; patch_z++)
		{
			for(int patch_x = centre_patch_x - 1; patch_x <= centre_patch_x + 1; patch_x++)
			{
				patch *p = _patch_mgr->get_patch(patch_x, patch_z);

				fw::matrix trans = fw::translation(fw::vector(
					(float) (patch_x * patch_manager::PATCH_SIZE) - p->get_origin()[0],
					0,
					(float) (patch_z * patch_manager::PATCH_SIZE) - p->get_origin()[2]));

				std::list< weak_ptr<entity> > patch_entities = p->get_entities();
				for(std::list< weak_ptr<entity> >::iterator it = patch_entities.begin();
					it != patch_entities.end(); ++it)
				{
					shared_ptr<entity> entity = (*it).lock();
					if (!entity) continue;

					entity->render(scenegraph, trans);
				}
			}
		}
	}
}
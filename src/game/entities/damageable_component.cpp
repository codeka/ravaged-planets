//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "entity_factory.h"
#include "entity_attribute.h"
#include "entity_manager.h"
#include "damageable_component.h"
#include "position_component.h"

namespace ent {

	// register the damageable component with the entity_factory
	ENT_COMPONENT_REGISTER("damageable", damageable_component);

	damageable_component::damageable_component()
	{
	}

	damageable_component::~damageable_component()
	{
	}

	void damageable_component::apply_template(shared_ptr<entity_component_template> comp_template)
	{
		BOOST_FOREACH(entity_component_template::property_map::value_type &kvp, comp_template->properties)
		{
			if (kvp.first == "Explosion")
			{
				_expl_name = kvp.second;
			}
		}
	}

	void damageable_component::initialise()
	{
		shared_ptr<entity> entity(_entity);
		entity_attribute *health = entity->get_attribute("health");
		if (health != 0)
		{
			health->sig_value_changed.connect(boost::bind(&damageable_component::check_explode, this, _2));
		}
	}

	void damageable_component::apply_damage(float amt)
	{
		shared_ptr<entity> entity(_entity);
		entity_attribute *attr = entity->get_attribute("health");
		if (attr != 0)
		{
			int curr_value = attr->get_value<int>();
			if (curr_value > 0)
			{
				attr->set_value(curr_value - static_cast<int>(amt));
			}
		}
	}

	// this is called whenever our health attribute changes value. we check whether it's
	// hit 0, and explode if it has
	void damageable_component::check_explode(boost::any health_value)
	{
		if (boost::any_cast<int>(health_value) <= 0)
		{
			explode();
		}
	}

	void apply_damage(shared_ptr<entity> ent, float amt)
	{
		damageable_component *damageable = ent->get_component<damageable_component>();
		if (damageable != 0)
		{
			damageable->apply_damage(amt);
		}
	}

	void damageable_component::explode()
	{
		shared_ptr<entity> entity(_entity); // entity is always valid while we're valid...
		entity->get_manager()->destroy(_entity);

		if (_expl_name != "")
		{
			// create an explosion entity
			entity->get_manager()->create_entity(entity, _expl_name, 0);
		}

		position_component *our_position = entity->get_component<position_component>();
		if (our_position != 0)
		{
			std::list< weak_ptr<ent::entity> > entities;
			our_position->get_entities_within_radius(5.0f, std::back_inserter(entities));
			BOOST_FOREACH(weak_ptr<ent::entity> const &wp, entities)
			{
				shared_ptr<ent::entity> ent = wp.lock();

				// don't damange ourselves...
				if (!ent || ent == entity)
					continue;

				ent::apply_damage(ent, (5.0f - our_position->get_direction_to(ent).length()));
			}
		}
	}

}
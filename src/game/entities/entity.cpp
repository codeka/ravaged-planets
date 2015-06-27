//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "entity.h"
#include "entity_debug.h"
#include "entity_factory.h"
#include "position_component.h"
#include "moveable_component.h"
#include "../../framework/framework.h"
#include "../../framework/graphics.h"
#include "../../framework/vector.h"
#include "../../framework/xml.h"
#include "../../framework/logging.h"
#include "../../framework/timer.h"
#include "../../framework/exception.h"

namespace ent {

	entity::entity(entity_manager *mgr, entity_id id)
		: _mgr(mgr), _debug_view(0), _debug_flags(static_cast<entity_debug_flags>(0))
		, _id(id)
	{
	}

	entity::~entity()
	{
		BOOST_FOREACH(component_map::value_type &pair, _components)
		{
			delete pair.second;
		}
	}

	void entity::add_component(entity_component *comp)
	{
		// you can only have one component of each type (we might want to change that...)
		component_map::iterator it = _components.find(comp->get_identifier());
		if (it != _components.end())
			BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("only one component of each type is allowed"));

		_components[comp->get_identifier()] = comp;
	}

	entity_component *entity::get_component(int identifier)
	{
		component_map::iterator it = _components.find(identifier);
		if (it == _components.end())
			return 0;

		return (*it).second;
	}

	bool entity::contains_component(int identifier) const
	{
		return (_components.find(identifier) != _components.end());
	}

	void entity::add_attribute(entity_attribute const &attr)
	{
		// you can only have one attribute with a given name
		attribute_map::iterator it = _attributes.find(attr.get_name());
		if (it != _attributes.end())
			BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("only one attribute with the same name is allowed"));

		_attributes[attr.get_name()] = attr;
	}

	entity_attribute *entity::get_attribute(std::string const &name)
	{
		attribute_map::iterator it = _attributes.find(name);
		if (it == _attributes.end())
			return 0;

		return &(*it).second;
	}

	void entity::initialise()
	{
		_create_time = fw::framework::get_instance()->get_timer()->get_total_time();
		BOOST_FOREACH(component_map::value_type &pair, _components)
		{
			pair.second->initialise();
		}
	}

	void entity::update(float dt)
	{
		BOOST_FOREACH(component_map::value_type &pair, _components)
		{
			pair.second->update(dt);
		}
	}

	void entity::render(fw::sg::scenegraph &scenegraph, fw::matrix const &transform)
	{
		BOOST_FOREACH(component_map::value_type &pair, _components)
		{
			pair.second->render(scenegraph, transform);
		}

		if (_debug_view != 0)
		{
			_debug_view->render(scenegraph, transform);
		}
	}

	void entity::set_position(fw::vector const &pos)
	{
		position_component *position = get_component<ent::position_component>();
		if (position != 0)
		{
			position->set_position(pos);

			moveable_component *moveable = get_component<ent::moveable_component>();
			if (moveable != 0)
			{
				moveable->set_goal(position->get_position());
			}
		}
	}

	float entity::get_age() const
	{
		float curr_time = fw::framework::get_instance()->get_timer()->get_total_time();
		return (curr_time - _create_time);
	}

	// gets the entity_debug_view object that contains the list of lines and points that
	// we'll draw along with this entity for debugging purposes.
	entity_debug_view *entity::get_debug_view()
	{
		if (_debug_flags != 0)
		{
			if (_debug_view == 0)
				_debug_view = new entity_debug_view();

			return _debug_view;
		}
		else
		{
			delete _debug_view;
			return 0;
		}
	}

	//-------------------------------------------------------------------------

	entity_component::entity_component()
	{
	}

	entity_component::~entity_component()
	{
	}

	// creates an instance of the entity_component_template we'll want to use (some components can have
	// special requirements for their templates that's not satified by the default implementation)
	entity_component_template *entity_component::create_template()
	{
		return new entity_component_template();
	}

}
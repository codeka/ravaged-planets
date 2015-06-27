//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "entity_factory.h"
#include "buildable_component.h"

namespace ent {

	// register the buildable component with the entity_factory
	ENT_COMPONENT_REGISTER("buildable", buildable_component);

	buildable_component::buildable_component()
	{
	}

	buildable_component::~buildable_component()
	{
	}

	void buildable_component::apply_template(shared_ptr<entity_component_template> comp_template)
	{
		BOOST_FOREACH(entity_component_template::property_map::value_type &kvp, comp_template->properties)
		{
			if (kvp.first == "BuildGroup")
			{
				_build_group = kvp.second;
			}
		}

		entity_component::apply_template(comp_template);
	}
}
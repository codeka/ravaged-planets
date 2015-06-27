//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "entity_factory.h"
#include "minimap_visible_component.h"

namespace ent {

	// register the minimap-visible component with the entity_factory
	ENT_COMPONENT_REGISTER("minimap-visible", minimap_visible_component);

	minimap_visible_component::minimap_visible_component()
	{
	}

	minimap_visible_component::~minimap_visible_component()
	{
	}

}
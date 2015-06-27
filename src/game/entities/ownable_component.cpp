//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "entity_factory.h"
#include "ownable_component.h"
#include "../simulation/local_player.h"
#include "../ai/ai_player.h"
#include "../simulation/simulation_thread.h"

namespace ent {

	// register the ownable component with the entity_factory
	ENT_COMPONENT_REGISTER("ownable", ownable_component);

	ownable_component::ownable_component()
		: _owner(0)
	{
	}

	ownable_component::~ownable_component()
	{
	}

	void ownable_component::set_owner(ww::player *owner)
	{
		_owner = owner;
		owner_changed_event(this);
	}

	bool ownable_component::is_local_player() const
	{
		if (_owner == 0)
			return false;

		ww::player *local_player = ww::simulation_thread::get_instance()->get_local_player();
		return (local_player == _owner);
	}

	bool ownable_component::is_local_or_ai_player() const
	{
		if (_owner == 0)
			return false;

		if (is_local_player())
			return true;

		ww::ai_player *ai_owner = dynamic_cast<ww::ai_player *>(_owner);
		if (ai_owner != 0)
			return true;

		return false;
	}

}
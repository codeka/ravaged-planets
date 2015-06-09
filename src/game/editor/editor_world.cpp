//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "editor_world.h"
#include "editor_terrain.h"
#include "../framework/graphics.h"
#include "../framework/exception.h"

namespace ed {

	world_create::world_create()
		: _width(-1), _height(-1)
	{
	}

	world_create::world_create(int width, int height)
		: _width(width), _height(height)
	{
		_terrain = create_terrain(width, height);
	}

	ww::terrain *world_create::create_terrain(int width, int length)
	{
		editor_terrain *et = new editor_terrain();
		et->create(width, length);
		et->initialise_splatt();
		return et;
	}

	//-------------------------------------------------------------------------

	editor_world::editor_world(shared_ptr<ww::world_reader> reader)
		: world(reader)
	{
	}

	editor_world::~editor_world()
	{
	}

	void editor_world::initialise_entities()
	{
		// no entities!
	}

	void editor_world::initialise_pathing()
	{
		// no pathing
	}

}

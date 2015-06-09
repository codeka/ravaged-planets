//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//

#include "stdafx.h"
#include "tools.h"
#include "../../framework/framework.h"
#include "../../framework/input.h"
#include "../../framework/scenegraph.h"
#include "../../framework/vertex_buffer.h"
#include "../../framework/index_buffer.h"
#include "../../framework/vertex_formats.h"
#include "../../game/application.h"
#include "../../game/world/world.h"
#include "../editor_terrain.h"
#include "../editor_screen.h"

namespace ed {

	tool::tool(ww::world *wrld)
		: _world(wrld), _terrain(0), _editor(0)
	{
	}

	tool::~tool()
	{
	}

	void tool::activate()
	{
		ww::application *app = dynamic_cast<ww::application *>(fw::framework::get_instance()->get_app());
		_editor = dynamic_cast<editor_screen *>(app->get_screen()->get_active_screen());
		_terrain = dynamic_cast<editor_terrain *>(_world->get_terrain());
	}

	void tool::deactivate()
	{
		// Loop through the keybind tokens and unbind them all
		fw::input *input = fw::framework::get_instance()->get_input();
		std::for_each(_keybind_tokens.begin(), _keybind_tokens.end(),
			boost::bind(&fw::input::unbind_key, input, _1));
	}

	void tool::update()
	{
	}

	void tool::render(fw::sg::scenegraph &)
	{
	}

	// This is used by a number of of the tools for giving a basic indication of
	// it's area of effect.
	void draw_circle(fw::sg::scenegraph &scenegraph, ww::terrain *terrain,
		fw::vector const &centre, float radius)
	{
		// the number of segments is basically the diameter of our circle. That means
		// we'll have one segment per unit, approximately.
		int num_segments = (int)(2.0f * M_PI * radius);

		// at least 8 segments, though...
		if (num_segments < 8)
			num_segments = 8;

		shared_ptr<fw::vertex_buffer> vb(new fw::vertex_buffer());
		vb->create_buffer<fw::vertex::xyz>(num_segments + 1);
		fw::vertex::xyz *vertices = new fw::vertex::xyz[num_segments + 1];

		for(int i = 0; i < num_segments; i++)
		{
			float factor = 2.0f * (float) M_PI * (i / (float) num_segments);
			vertices[i].x = centre[0] + radius * sin(factor);
			vertices[i].z = centre[2] + radius * cos(factor);
			vertices[i].y = terrain->get_height(vertices[i].x, vertices[i].z) + 0.5f;
		}

		vertices[num_segments].x = vertices[0].x;
		vertices[num_segments].y = vertices[0].y;
		vertices[num_segments].z = vertices[0].z;

		vb->set_data(num_segments + 1, vertices);

		shared_ptr<fw::sg::node> node(new fw::sg::node());
		node->set_vertex_buffer(vb);
		node->set_primitive_type(D3DPT_LINESTRIP);
		scenegraph.add_node(node);
	}

	//-------------------------------------------------------------------------
	typedef std::map<std::string, create_tool_fn> tool_map;
	static tool_map *g_tools = 0;

	tool_factory_registrar::tool_factory_registrar(std::string const &name, create_tool_fn fn)
	{
		if (g_tools == 0)
		{
			g_tools = new tool_map();
		}

		(*g_tools)[name] = fn;
	}

	tool *tool_factory::create_tool(std::string const &name, ww::world *world)
	{
		if (g_tools == 0)
			return 0;

		tool_map::iterator it = g_tools->find(name);
		if (it == g_tools->end())
			return 0;

		return (*it).second(world);
	}

}

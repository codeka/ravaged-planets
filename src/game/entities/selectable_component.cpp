//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "entity.h"
#include "entity_factory.h"
#include "selectable_component.h"
#include "position_component.h"
#include "ownable_component.h"
#include "../../framework/framework.h"
#include "../../framework/graphics.h"
#include "../../framework/vertex_buffer.h"
#include "../../framework/index_buffer.h"
#include "../../framework/vertex_formats.h"
#include "../../framework/effect.h"
#include "../../framework/texture.h"
#include "../../framework/scenegraph.h"
#include "../../framework/vector.h"
#include "../world/world.h"
#include "../world/terrain.h"

namespace ent {

	static shared_ptr<fw::effect> _fx;
	static shared_ptr<fw::vertex_buffer> _vb;
	static shared_ptr<fw::index_buffer> _ib;

	// register the selectable component with the entity_factory
	ENT_COMPONENT_REGISTER("selectable", selectable_component);

	selectable_component::selectable_component()
		: _is_selected(false), _selection_radius(2.0f), _is_highlighted(false), _ownable(0)
	{
	}

	selectable_component::~selectable_component()
	{
	}

	void selectable_component::initialise()
	{
		// grab a reference to the ownable component of our entity so we can refer to it
		// later on.
		shared_ptr<entity> entity(_entity);
		_ownable = entity->get_component<ownable_component>();

		if (!_vb)
		{
			populate_buffers();
			fw::framework::get_instance()->get_graphics()->sig_device_lost.connect(&populate_buffers);
		}
	}

	// this is called when we start up, and also when our device is reset. we need to populate
	// the vertex buffer and index buffer
	void selectable_component::populate_buffers()
	{
		shared_ptr<fw::vertex_buffer> vb(new fw::vertex_buffer());
		vb->create_buffer<fw::vertex::xyz_uv>(4);
		fw::vertex::xyz_uv vertices[4] = {
			fw::vertex::xyz_uv(-1.0f, 0.0f, -1.0f, 0.0f, 0.0f),
			fw::vertex::xyz_uv(-1.0f, 0.0f,  1.0f, 0.0f, 1.0f),
			fw::vertex::xyz_uv( 1.0f, 0.0f,  1.0f, 1.0f, 1.0f),
			fw::vertex::xyz_uv( 1.0f, 0.0f, -1.0f, 1.0f, 0.0f)
		};
		vb->set_data(4, vertices);
		_vb = vb;

		shared_ptr<fw::index_buffer> ib(new fw::index_buffer());
		ib->create_buffer(6, D3DFMT_INDEX16);
		uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };
		ib->set_data(6, indices);
		_ib = ib;

		if (!_fx)
		{
			shared_ptr<fw::effect> fx(new fw::effect());
			fx->initialise("selection.fx");
			_fx = fx;
		}
	}

	void selectable_component::set_is_selected(bool selected)
	{
		 _is_selected = selected;
		 sig_selected(selected);
	}

	void selectable_component::apply_template(shared_ptr<entity_component_template> comp_template)
	{
		BOOST_FOREACH(entity_component_template::property_map::value_type &kvp, comp_template->properties)
		{
			if (kvp.first == "SelectionRadius")
			{
				set_selection_radius(boost::lexical_cast<float>(kvp.second));
			}
		}

		entity_component::apply_template(comp_template);
	}

	void selectable_component::set_selection_radius(float value)
	{
		_selection_radius = value;
	}

	void selectable_component::highlight(fw::colour const &col)
	{
		_is_highlighted = true;
		_highlight_colour = col;
	}

	void selectable_component::unhighlight()
	{
		_is_highlighted = false;
	}


	void selectable_component::render(fw::sg::scenegraph &scenegraph, fw::matrix const &transform)
	{
		bool draw = false;
		fw::colour col(1,1,1);
		if (_is_selected)
		{
			draw = true;
			col = fw::colour(1, 1, 1);
		}
		else if (_is_highlighted)
		{
			draw = true;
			col = _highlight_colour;
		}

		if (!draw)
			return;

		shared_ptr<entity> entity(_entity);
		position_component *pos = entity->get_component<position_component>();
		if (pos != 0)
		{
			shared_ptr<fw::effect_parameters> fx_params = _fx->create_parameters();
			fx_params->set_colour("selection_colour", col);
			fx_params->commit();

			fw::matrix m = pos->get_transform() * transform;
			m *= fw::translation(fw::vector(0.0f, 0.2f, 0.0f)); // lift it off the ground a bit
			m = fw::scale(_selection_radius) * m; // scale it to the size of our selection radius

			shared_ptr<fw::sg::node> node(new fw::sg::node());
			node->set_vertex_buffer(_vb);
			node->set_index_buffer(_ib);
			node->set_effect(_fx);
			node->set_effect_parameters(fx_params);
			node->set_world_matrix(m);
			node->set_primitive_type(D3DPT_TRIANGLELIST);
			node->set_cast_shadows(false);
			scenegraph.add_node(node);
		}
	}
}
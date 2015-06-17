//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "model.h"
#include "model_node.h"
#include "effect.h"
#include "texture.h"
#include "colour.h"
#include "graphics.h"

namespace fw {

	model_node::model_node()
		: wireframe(false), transform(fw::identity())
	{
	}

	model_node::~model_node()
	{
	}

	void model_node::initialise(model &mdl)
	{
		if (mesh_index >= 0)
		{
			shared_ptr<model_mesh> mesh = mdl.meshes[mesh_index];
			set_vertex_buffer(mesh->get_vertex_buffer());
			set_index_buffer(mesh->get_index_buffer());
			set_effect(mesh->get_effect());
			set_primitive_type(D3DPT_TRIANGLELIST);

			if (mdl.texture)
			{
				shared_ptr<effect_parameters> params = get_effect()->create_parameters();
				params->set_texture("entity_texture", mdl.texture);
				params->set_colour("mesh_colour", fw::colour(1, 1, 1));
				params->commit();
				set_effect_parameters(params);
			}
		}

		BOOST_FOREACH(shared_ptr<node> node, _children)
		{
			boost::dynamic_pointer_cast<model_node>(node)->initialise(mdl);
		}
	}

	void model_node::render(sg::scenegraph *sg)
	{
		IDirect3DDevice9 *device = 0;

		if (wireframe)
		{
			device = fw::framework::get_instance()->get_graphics()->get_device();
			device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		}

		fw::matrix old_world = _world;
		_world = transform * _world;

		node::render(sg);

		_world = old_world;

		if (wireframe)
		{
			device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		}
	}

	// called to set any additional parameters on the given effect
	void model_node::setup_effect(shared_ptr<fw::effect> fx)
	{
		fx->set_colour("mesh_colour", colour);
	}

	void model_node::populate_clone(shared_ptr<fw::sg::node> clone)
	{
		node::populate_clone(clone);

		shared_ptr<model_node> mnclone(boost::shared_dynamic_cast<model_node>(clone));
		mnclone->mesh_index = mesh_index;
		mnclone->node_name = node_name;
		mnclone->wireframe = wireframe;
		mnclone->colour = colour;
		mnclone->transform = transform;
	}

	shared_ptr<fw::sg::node> model_node::clone()
	{
		shared_ptr<fw::sg::node> clone(new model_node());
		populate_clone(clone);
		return clone;
	}

}
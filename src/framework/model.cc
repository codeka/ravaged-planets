//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "model.h"
#include "model_node.h"
#include "scenegraph.h"
#include "framework.h"
#include "graphics.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "effect.h"

namespace fw {

	model_mesh::model_mesh(int /*num_vertices*/, int /*num_indices*/)
	{
	}

	model_mesh::~model_mesh()
	{
	}

	//-------------------------------------------------------------------------

	model_mesh_noanim::model_mesh_noanim(int num_vertices, int num_indices)
		: model_mesh(num_vertices, num_indices)
		, vertices(num_vertices), indices(num_indices)
	{
	}

	model_mesh_noanim::~model_mesh_noanim()
	{
	}

	void model_mesh_noanim::setup_buffers()
	{
		if (_vb)
			return;

		shared_ptr<vertex_buffer> vb(new vertex_buffer());
		vb->create_buffer<vertex::xyz_n_uv>(vertices.size());
		vb->set_data(vertices.size(), &vertices[0]);
		_vb = vb;

		shared_ptr<index_buffer> ib(new index_buffer());
		ib->create_buffer(indices.size(), D3DFMT_INDEX16);
		ib->set_data(indices.size(), &indices[0]);
		_ib = ib;

		shared_ptr<effect> fx(new effect());
		fx->initialise("entity.fx");
		_fx = fx;
	}

	//-------------------------------------------------------------------------

	model::model()
	{
	}

	model::~model()
	{
	}

	void model::render(sg::scenegraph &sg, fw::matrix const &transform /*= fw::matrix::identity() */)
	{
		root_node->set_world_matrix(transform);
		sg.add_node(root_node->clone());
	}

	void model::set_wireframe(bool value)
	{
		root_node->wireframe = value;
	}

	bool model::get_wireframe() const
	{
		return root_node->wireframe;
	}

	void model::set_colour(fw::colour col)
	{
		root_node->colour = col;
	}

	fw::colour model::get_colour() const
	{
		return root_node->colour;
	}

}
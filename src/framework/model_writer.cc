//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "model.h"
#include "model_writer.h"
#include "model_file.h"
#include "model_node.h"
#include "exception.h"

namespace fw {
	
	void write_mesh(model_file &file, shared_ptr<model_mesh> mesh);
	void write_node(model_file &file, shared_ptr<model_node> node);

	void model_writer::write(std::string const &filename, shared_ptr<model> mdl)
	{
		write(filename, *mdl.get());
	}

	// writes the given model out to the given file
	void model_writer::write(std::string const &filename, model &mdl)
	{
		model_file file(filename, true);
		file.write_header();

		BOOST_FOREACH(shared_ptr<model_mesh> mesh, mdl.meshes)
		{
			write_mesh(file, mesh);
		}
		write_node(file, mdl.root_node);
	}

	void write_mesh(model_file &file, shared_ptr<model_mesh> mesh)
	{
		model_file_chunk chunk;
		chunk.chunk_id = MDLCHNK_MESH;
		chunk.body_bytes = sizeof(model_file_mesh);
		chunk.num_children = 2;
		file.write(chunk);

		model_file_mesh mfm;
		shared_ptr<model_mesh_noanim> mesh_noanim(boost::shared_dynamic_cast<model_mesh_noanim>(mesh));
		if (mesh_noanim)
		{
			mfm.num_indices = mesh_noanim->indices.size();
			mfm.num_vertices = mesh_noanim->vertices.size();
			file.write(mfm);

			chunk.chunk_id = MDLCHNK_INDICES;
			chunk.body_bytes = mesh_noanim->indices.size() * sizeof(uint16_t);
			chunk.num_children = 0;
			file.write(chunk);
			file.write(reinterpret_cast<char const *>(&mesh_noanim->indices[0]), mesh_noanim->indices.size() * sizeof(uint16_t));

			chunk.chunk_id = MDLCHNK_VERTICES;
			chunk.body_bytes = mesh_noanim->vertices.size() * sizeof(fw::vertex::xyz_n_uv);
			chunk.num_children = 0;
			file.write(chunk);
			file.write(reinterpret_cast<char const *>(&mesh_noanim->vertices[0]), mesh_noanim->vertices.size() * sizeof(fw::vertex::xyz_n_uv));
		}
		else
		{
			BOOST_THROW_EXCEPTION(fw::exception()
				<< fw::message_error_info("Only mesh_noanim is supported at the moment..."));
		}
	}

	void write_node(model_file &file, shared_ptr<model_node> node)
	{
		model_file_chunk chunk;
		chunk.chunk_id = MDLCHNK_NODE;
		chunk.body_bytes = sizeof(model_file_node);
		chunk.num_children = node->get_num_children();

		model_file_node file_node;
		memset(&file_node, 0, sizeof(model_file_node));
		file_node.mesh_index = node->mesh_index;
		for(int i = 0; i < 16; i++)
		{
			file_node.transformation[i] = node->transform.data()[i];
		}
		strcpy_s(file_node.name, node->node_name.c_str());

		file.write(chunk);
		file.write(file_node);

		for(int i = 0; i < node->get_num_children(); i++)
		{
			shared_ptr<model_node> child_node = boost::dynamic_pointer_cast<model_node>(node->get_child(i));
			write_node(file, child_node);
		}
	}

}

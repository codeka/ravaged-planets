//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "model.h"
#include "model_reader.h"
#include "model_file.h"
#include "model_node.h"
#include "texture.h"
#include "framework.h"
#include "graphics.h"
#include "exception.h"

#include <limits>

namespace fs = boost::filesystem;

namespace fw {

	shared_ptr<model_mesh> read_mesh(model_file_chunk &chunk, model_file &file);
	shared_ptr<model_node> read_node(model_file_chunk &chunk, model_file &file);

	shared_ptr<model> model_reader::read(fs::path const &filename)
	{
		shared_ptr<model> model(new model());

		model_file file(filename, false);
		file.read_header();

		while (!file.at_eof())
		{
			model_file_chunk chunk;
			file.read(chunk);
			if (chunk.chunk_id == MDLCHNK_MESH)
			{
				shared_ptr<model_mesh> mesh = read_mesh(chunk, file);
				model->meshes.push_back(mesh);
			}
			else if (chunk.chunk_id = MDLCHNK_NODE)
			{
				if (model->root_node)
				{
					BOOST_THROW_EXCEPTION(fw::exception()
						<< fw::message_error_info("More than one root node found!")
						<< fw::filename_error_info(file.get_filename()));
				}

				shared_ptr<model_node> node = read_node(chunk, file);
				model->root_node = node;
			}
			else
			{
				BOOST_THROW_EXCEPTION(fw::exception()
					<< fw::message_error_info("Unknown mesh child: " + boost::lexical_cast<std::string>(chunk.chunk_id))
					<< fw::filename_error_info(file.get_filename()));
			}
		}

		// load the texture, if there is one (currently .png files)
		fs::path texture_path = fs::path(filename).replace_extension("png");
		if (fs::is_regular_file(texture_path))
		{
			model->texture = shared_ptr<texture>(new texture());
			model->texture->create(texture_path);
		}

		if (!model->root_node)
		{
				BOOST_THROW_EXCEPTION(fw::exception()
					<< fw::message_error_info("No root node defined!")
					<< fw::filename_error_info(file.get_filename()));
		}
		model->root_node->initialise(*model.get());

		return model;
	}

	shared_ptr<model_mesh> read_mesh(model_file_chunk &chunk, model_file &file)
	{
		model_file_mesh mfm;
		file.read(mfm);

		shared_ptr<model_mesh_noanim> mesh(new model_mesh_noanim(mfm.num_vertices, mfm.num_indices));
		for(uint16_t i = 0; i < chunk.num_children; i++)
		{
			model_file_chunk child;
			file.read(child);

			if (child.chunk_id == MDLCHNK_INDICES)
			{
				file.read(reinterpret_cast<char *>(&mesh->indices[0]), child.body_bytes);
			}
			else if (child.chunk_id == MDLCHNK_VERTICES)
			{
				file.read(reinterpret_cast<char *>(&mesh->vertices[0]), child.body_bytes);
			}
			else
			{
				BOOST_THROW_EXCEPTION(fw::exception()
					<< fw::message_error_info("Unknown mesh child: " + boost::lexical_cast<std::string>(child.chunk_id))
					<< fw::filename_error_info(file.get_filename()));
			}
		}

		return mesh;
	}

	shared_ptr<model_node> read_node(model_file_chunk &chunk, model_file &file)
	{
		model_file_node mfn;
		file.read(mfn);

		shared_ptr<model_node> node(new model_node());
		node->mesh_index = mfn.mesh_index == std::numeric_limits<uint16_t>::max()
						   ? -1
						   : mfn.mesh_index;
		node->node_name = mfn.name;
		for(int i = 0; i < 16; i++)
		{
			node->transform.data()[i] = mfn.transformation[i];
		}

		for(int i = 0; i < chunk.num_children; i++)
		{
			model_file_chunk child;
			file.read(child);

			if (chunk.chunk_id = MDLCHNK_NODE)
			{
				shared_ptr<model_node> child_node = read_node(child, file);
				node->add_child(boost::dynamic_pointer_cast<sg::node>(child_node));
			}
			else
			{
				BOOST_THROW_EXCEPTION(fw::exception()
					<< fw::message_error_info("Unknown node child: " + boost::lexical_cast<std::string>(child.chunk_id))
					<< fw::filename_error_info(file.get_filename()));
			}
		}

		return node;
	}

}

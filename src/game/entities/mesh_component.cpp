//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "entity.h"
#include "entity_factory.h"
#include "position_component.h"
#include "mesh_component.h"
#include "ownable_component.h"
#include "../simulation/player.h"
#include "../../framework/graphics.h"
#include "../../framework/model.h"
#include "../../framework/model_reader.h"
#include "../../framework/misc.h"
#include "../../framework/logging.h"
#include "../../framework/scenegraph.h"

namespace fs = boost::filesystem;

namespace ent {

	// register the mesh component with the entity_factory
	ENT_COMPONENT_REGISTER("mesh", mesh_component);

	mesh_component::mesh_component()
		: _model(), _colour(1, 1, 1)
	{
	}

	mesh_component::mesh_component(shared_ptr<fw::model> const &model)
		: _model(model)
	{
	}

	mesh_component::~mesh_component()
	{
	}

	// this is called when the entity's owner changes
	void mesh_component::owner_changed(ownable_component *ownable)
	{
		ww::player *plyr = ownable->get_owner();
		if (plyr != 0)
		{
			_colour = plyr->get_colour();

			if (_model)
			{
				_model->set_colour(_colour);
			}
		}
	}

	void mesh_component::apply_template(shared_ptr<entity_component_template> comp_template)
	{
		BOOST_FOREACH(entity_component_template::property_map::value_type &kvp, comp_template->properties)
		{
			if (kvp.first == "FileName")
			{
				fs::path file_name = fw::installed_data_path() / "meshes" / kvp.second;

				fw::model_reader mdl_reader;
				shared_ptr<fw::model> model = mdl_reader.read(file_name);

				set_model(model);
			}
		}

		entity_component::apply_template(comp_template);
	}

	void mesh_component::initialise()
	{
		shared_ptr<entity> entity(_entity);
		ownable_component *ownable = entity->get_component<ownable_component>();
		if (ownable != 0)
		{
			// get a signal if/when the owner changes
			ownable->owner_changed_event.connect(boost::bind(&mesh_component::owner_changed, this, _1));
			owner_changed(ownable);
		}
	}

	void mesh_component::set_model(shared_ptr<fw::model> const &model)
	{
		_model = model;
		_model->set_colour(_colour);
	}

	void mesh_component::render(fw::sg::scenegraph &scenegraph, fw::matrix const &transform)
	{
		shared_ptr<entity> entity(_entity);
		position_component *pos = entity->get_component<position_component>();
		if (pos != 0)
		{
			_model->render(scenegraph, pos->get_transform() * transform);
		}
	}
}
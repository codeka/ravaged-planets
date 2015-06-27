//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "entity_factory.h"
#include "builder_component.h"
#include "selectable_component.h"
#include "position_component.h"
#include "ownable_component.h"
#include "../screens/hud/build_window.h"
#include "../screens/hud/chat_window.h"
#include "../simulation/simulation_thread.h"
#include "../simulation/commands.h"

namespace ent {

	// register the builder component with the entity_factory
	ENT_COMPONENT_REGISTER("builder", builder_component);

	builder_component::builder_component()
		: _time_to_build(0.0f)
	{
	}

	builder_component::~builder_component()
	{
	}

	void builder_component::initialise()
	{
		shared_ptr<entity> entity(_entity);
		selectable_component *sel = entity->get_component<selectable_component>();
		if (sel != 0)
		{
			sel->sig_selected.connect(boost::bind(&builder_component::on_selected, this, _1));
		}
	}

	void builder_component::apply_template(shared_ptr<entity_component_template> comp_template)
	{
		BOOST_FOREACH(entity_component_template::property_map::value_type &kvp, comp_template->properties)
		{
			if (kvp.first == "BuildGroup")
			{
				_build_group = kvp.second;
			}
		}

		entity_component::apply_template(comp_template);
	}

	// this is called when our entity is selected/deselected, we need to show/hide the build window
	// as appropriate
	void builder_component::on_selected(bool selected)
	{
		if (selected)
		{
			ww::hud_build->show();
			ww::hud_build->refresh(_entity, _build_group);
		}
		else
		{
			ww::hud_build->hide();
		}
	}

	void builder_component::build(std::string name)
	{
		ww::hud_chat->add_line("Building: " + name + "...");

		entity_factory factory;
		_curr_building = factory.get_template(name);
		_time_to_build = 5.0f; // todo: get this from the template
	}

	bool builder_component::is_building() const
	{
		return !!_curr_building;
	}

	void builder_component::update(float dt)
	{
		if (_curr_building)
		{
			_time_to_build -= dt;

			if (_time_to_build <= 0.0f)
			{
				shared_ptr<entity> entity(_entity);
				ownable_component *our_ownable = entity->get_component<ownable_component>();
				if (our_ownable != 0 && our_ownable->is_local_or_ai_player())
				{
					position_component *our_pos = entity->get_component<position_component>();
					if (our_pos != 0)
					{
						shared_ptr<ww::create_entity_command> cmd(ww::create_command<ww::create_entity_command>());
						cmd->template_name = _curr_building->name;
						cmd->initial_position = our_pos->get_position();
						cmd->initial_goal = our_pos->get_position() + (our_pos->get_direction() * 3.0f);
						ww::simulation_thread::get_instance()->post_command(cmd);
					}
				}

				_curr_building.reset();
				_time_to_build = 0.0f;
			}
		}
	}

}
//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "particle.h"
#include "particle_config.h"
#include "texture.h"
#include "logging.h"
#include "misc.h"

namespace fw {

	//-------------------------------------------------------------------------
	particle::life_state::life_state()
	{
	}

	particle::life_state::life_state(particle::life_state const &copy)
	{
		age = copy.age;
		size = copy.size;
		colour_row = copy.colour_row;
		alpha = copy.alpha;
		rotation_speed = copy.rotation_speed;
		rotation_kind = copy.rotation_kind;
		speed = copy.speed;
		gravity = copy.gravity;
	}

	//-------------------------------------------------------------------------

	particle::particle(shared_ptr<particle_emitter_config> const &config)
		: config(config)
	{
		random = fw::random();
		angle = 0.0f;
		size = 0.0f;
		draw_frame = 0;

		if (config->billboard.areas.size() < 1)
		{
			rect.left = rect.top = 0.0f;
			rect.right = rect.bottom = 1.0f;
		}
		else
		{
			int index = static_cast<int>(random * (config->billboard.areas.size() - 1));
			rect = config->billboard.areas[index];
		}
	}

	particle::~particle()
	{
	}

	void particle::initialise()
	{
		_max_age = config->max_age.get_value();
		_states.clear();
		for(std::vector<particle_emitter_config::life_state>::const_iterator it = config->life.begin();
			it != config->life.end(); it++)
		{
			life_state state;
			state.age = (*it).age;
			state.size = (*it).size.get_value();
			state.colour_row = (*it).colour_row;
			state.alpha = (*it).alpha;
			state.rotation_speed = (*it).rotation_speed.get_value();
			state.rotation_kind = (*it).rotation_kind;
			state.speed = (*it).speed.get_value();
			state.gravity = (*it).gravity.get_value();
			_states.push_back(state);
		}

		direction = fw::vector(fw::random() - 0.5f, fw::random() - 0.5f, fw::random() - 0.5f).normalize();
		pos = config->position.get_point();
		age = 0.0f;
	}

	bool particle::update(float dt)
	{
		float ndt = dt / _max_age;
		age += ndt;
		if (age > 1.0f)
		{
			return false;
		}

		std::vector<life_state>::iterator prev_it;
		std::vector<life_state>::iterator next_it;
		for(prev_it = _states.begin(); prev_it != _states.end(); ++prev_it)
		{
			next_it = prev_it + 1;

			if (next_it == _states.end() || (*next_it).age > age)
				break;
		}

		float t;
		life_state const *next;
		life_state const *prev;

		if (next_it == _states.end())
		{
			next = &(*prev_it);
			prev = &(*prev_it);
			t = 0.0f;
		}
		else
		{
			next = &(*next_it);
			prev = &(*prev_it);
			
			// lerp between (last_it) and (next_it)
			float age_diff = next->age - prev->age;

			// t is a value between 0 and 1. when age == last_it.age, t will be
			// zero. When age == next_it.age, t will be one.
			t = (age - prev->age) / age_diff;
		}

		// adjust the particle's direction based on it's gravity factor
		fw::vector gravity = fw::vector(0, -1, 0) * (fw::lerp(prev->gravity, next->gravity, t));
		direction += gravity * dt;

		float rotation_speed = 0.0f;
		rotation_kind = prev->rotation_kind;
		if (rotation_kind == rotation_kind::random)
		{
			rotation_speed = fw::lerp(prev->rotation_speed, next->rotation_speed, t);
			angle += rotation_speed * dt;
		}

		float speed = fw::lerp(prev->speed, next->speed, t);
		colour1 = prev->colour_row;
		colour2 = next->colour_row;
		colour_factor = t;
		alpha = fw::lerp(prev->alpha, next->alpha, t);
		size = fw::lerp(prev->size, next->size, t);

		pos += (direction * speed) * dt;

		return true;
	}

}
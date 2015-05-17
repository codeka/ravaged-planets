//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "particle_manager.h"
#include "particle_emitter.h"
#include "particle.h"
#include "particle_config.h"
#include "misc.h"

namespace fw {

	particle_emitter::particle_emitter(particle_manager *mgr, shared_ptr<particle_emitter_config> config)
		: _mgr(mgr), _emit_policy(0), _config(config), _dead(false), _age(0.0f), _position(0, 0, 0)
	{
		if (_config->emit_policy_name == "distance")
		{
			_emit_policy = new distance_emit_policy(_config->emit_policy_value);
		}
		else if (_config->emit_policy_name == "timed")
		{
			_emit_policy = new timed_emit_policy(_config->emit_policy_value);
		}
		else // if (_config->emit_policy_name == "none")
		{
			_emit_policy = new no_emit_policy(_config->emit_policy_value);
		}
		
		_initial_count = _config->initial_count;
	}

	particle_emitter::~particle_emitter()
	{
	}

	void particle_emitter::initialise()
	{
		_emit_policy->initialise(this);
	}

	bool particle_emitter::update(float dt)
	{
		_age += dt;

		// if we're not supposed to have started yet, don't do anything
		if (_config->start_time > _age)
			return true;

		// if, on the other hand, we're supposed to be finished, mark ourselves dead
		if (_config->end_time > 0 && _age > _config->end_time)
			destroy();

		// check whether we need to emit a new particle
		if (!_dead)
		{
			while (_initial_count > 0)
			{
				emit(get_position());
				_initial_count --;
			}

			_emit_policy->check_emit(dt);
		}

		std::vector<particle_list::iterator> to_remove;

		// go through each particle and update it's various properties
		for(particle_list::iterator it = _particles.begin(); it != _particles.end(); ++it)
		{
			particle *p = *it;
			if (!p->update(dt))
				to_remove.push_back(it);
		}

		// remove any particles that are too old
		for(std::vector<particle_list::iterator>::iterator it = to_remove.begin(); it != to_remove.end(); ++it)
		{
			_mgr->remove_particle(*(*it));
			_particles.erase(*it);
		}

		return (!_dead || _particles.size() != 0);
	}

	void particle_emitter::destroy()
	{
		_dead = true;
	}

	// This is called when it's time to emit a new particle.
	// The offset is used when emitting "extra" particles, we need to offset
	// their age and position a bit
	particle *particle_emitter::emit(fw::vector pos,
									 float time_offset /*= 0.0f*/)
	{
		particle *p = new particle(_config);
		p->initialise();
		p->pos += pos;
		p->age = time_offset;

		_particles.push_back(p);
		_mgr->add_particle(p);

		return p;
	}

	//-------------------------------------------------------------------------

	void emit_policy::initialise(particle_emitter *emitter)
	{
		_emitter = emitter;
	}

	//-------------------------------------------------------------------------

	timed_emit_policy::timed_emit_policy(float value)
		: _particles_per_second(0.0f), _time_since_last_particle(0.0f)
		, _last_position(0, 0, 0)
	{
		_particles_per_second = value;
	}

	timed_emit_policy::~timed_emit_policy()
	{
	}

	void timed_emit_policy::check_emit(float dt)
	{
		float _seconds_per_particle = 1.0f / _particles_per_second;

		bool offset_pos = (_time_since_last_particle != 0.0f);

		_time_since_last_particle += dt;
		float time_offset = dt;

		fw::vector pos = _emitter->get_position();
		fw::vector dir = (pos - _last_position).normalize();

		while (_time_since_last_particle > _seconds_per_particle)
		{
			time_offset -= _seconds_per_particle;
			_time_since_last_particle -= _seconds_per_particle;

			fw::vector curr_pos = offset_pos
								? _last_position + (dir * _seconds_per_particle)
								: pos;

			_emitter->emit(curr_pos, time_offset);
		}

		_last_position = pos;
	}

	//-------------------------------------------------------------------------

	distance_emit_policy::distance_emit_policy(float value)
		: _max_distance(0.0f), _last_particle(0)
	{
		_max_distance = value;
	}

	distance_emit_policy::~distance_emit_policy()
	{
	}

	void distance_emit_policy::check_emit(float)
	{
		if (_last_particle == 0)
		{
			_last_particle = _emitter->emit(_emitter->get_position());
			return;
		}

		float wrap_x = _emitter->get_manager()->get_wrap_x();
		float wrap_z = _emitter->get_manager()->get_wrap_z();

		fw::vector next_pos = _emitter->get_position();
		fw::vector last_pos = _last_particle->pos;
		fw::vector dir = get_direction_to(last_pos, next_pos, wrap_x, wrap_z).normalize();
		fw::vector curr_pos = last_pos + (dir * _max_distance);

		float time_offset = 0.0f; // todo: this should be non-zero...

		float this_distance = calculate_distance(curr_pos, next_pos, wrap_x, wrap_z);
		float last_distance = this_distance + 1.0f;
		while (last_distance >= this_distance)
		{
			_emitter->emit(curr_pos, time_offset);

			curr_pos += dir * _max_distance;

			last_distance = this_distance;
			this_distance = (curr_pos - next_pos).length();
		}

		_last_particle = _emitter->emit(next_pos, time_offset);
	}

	no_emit_policy::no_emit_policy(float)
	{
	}

	no_emit_policy::~no_emit_policy()
	{
	}

	void no_emit_policy::check_emit(float)
	{
		// we destroy ourselves because we're actually never going to
		// do anything
		_emitter->destroy();
	}

}
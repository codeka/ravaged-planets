//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "particle_effect.h"
#include "particle_manager.h"
#include "particle_config.h"
#include "vector.h"

namespace fw {

	particle_effect::particle_effect(particle_manager *mgr, shared_ptr<particle_effect_config> const &config)
		: _mgr(mgr), _config(config), _dead(false)
	{
	}

	particle_effect::~particle_effect()
	{
	}

	void particle_effect::initialise()
	{
		for(particle_effect_config::emitter_config_list::iterator it = _config->emitter_config_begin();
			it != _config->emitter_config_end(); ++it)
		{
			shared_ptr<particle_emitter> emitter(new particle_emitter(_mgr, *it));
			emitter->initialise();
			_emitters.push_back(emitter);
		}
	}

	void particle_effect::destroy()
	{
		// destroy any emitters that are still running
		BOOST_FOREACH(emitter_list::value_type &emitter, _emitters)
		{
			emitter->destroy();
		}

		// and set our own "dead" flag. when all emitters have finished, we'll destroy ourselves.
		_dead = true;
	}

	void particle_effect::set_position(fw::vector const &pos)
	{
		BOOST_FOREACH(emitter_list::value_type &emitter, _emitters)
		{
			emitter->set_position(pos);
		}
	}

	void particle_effect::update(float dt)
	{
		std::vector<particle_emitter *> to_delete;

		BOOST_FOREACH(emitter_list::value_type &emitter, _emitters)
		{
			if (!emitter->update(dt))
			{
				to_delete.push_back(emitter.get());
			}
		}

		for(std::vector<particle_emitter *>::iterator dit = to_delete.begin(); dit != to_delete.end(); ++dit)
		{
			for(emitter_list::iterator it = _emitters.begin(); it != _emitters.end(); ++it)
			{
				if ((*it).get() == *dit)
				{
					_emitters.erase(it);
					break;
				}
			}
		}

		if (_dead && _emitters.size() == 0)
		{
			_mgr->remove_effect(this);
		}
	}

}

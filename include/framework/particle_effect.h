//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#pragma once

#include "particle_emitter.h"
#include "vector.h"

namespace fw {
	class particle_manager;
	class particle_effect_config;

	// particle effect represents, basically, a collection of particle_emitters
	// and is what you get when you "load" an effect.
	class particle_effect
	{
	public:
		typedef std::vector< shared_ptr<particle_emitter> > emitter_list;

	private:
		shared_ptr<particle_effect_config> _config;
		particle_manager *_mgr;
		emitter_list _emitters;
		bool _dead;

	public:
		particle_effect(particle_manager *mgr, shared_ptr<particle_effect_config> const &config);
		~particle_effect();

		void initialise();
		void destroy();

		void set_position(fw::vector const &pos);
		void update(float dt);

		bool is_dead() const { return _dead; }
	};

}
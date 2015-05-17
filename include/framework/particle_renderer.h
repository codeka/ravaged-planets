//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#pragma once

#include "vertex_formats.h"
struct render_state;

namespace fw {
	class effect;
	class graphics;
	class particle;
	class particle_manager;
	class vertex_buffer;
	class index_buffer;
	class texture;
	class effect;
	namespace sg {
		class scenegraph;
	}

	// This class is responsible for rendering particles. We create our own
	// special scenegraph node that does the main work.
	class particle_renderer
	{
	public:
		// This is the type of a list of particles. It must match the particle_list
		// type defined in particle_manager
		typedef std::list<particle *> particle_list;

	private:
		graphics *_graphics;
		shared_ptr<effect> _fx;
		shared_ptr<texture> _colour_texture;
		particle_manager *_mgr;
		int _draw_frame;

		void particle_renderer::render_particles(render_state &rs, float offset_x, float offset_z);
		bool add_particle(render_state &rs, int base_index, particle *p, float offset_x, float offset_z);

		// sort the particles so they're optimal for rendering
		void sort_particles(particle_renderer::particle_list &particles);

	public:
		particle_renderer(particle_manager *mgr);
		~particle_renderer();

		void initialise(graphics *g);
		void render(sg::scenegraph &scenegraph, particle_list &particles);
	};
}

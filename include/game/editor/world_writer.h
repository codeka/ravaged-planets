//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#pragma once

#include "../framework/colour.h"

namespace ww {
	class world_file;
}

namespace ed {
	class editor_world;

	// this class writes all the information for a given world (map) into the given directory
	// or .wwmap file (which is just a zipped version of the directory)
	class world_writer
	{
	private:
		editor_world *_world;
		std::string _name;
		fw::colour _base_minimap_colours[4];

		fw::colour get_terrain_colour(int x, int z);
		void calculate_base_minimap_colours();

		void write_terrain(ww::world_file &wf);
		void write_wwmap(ww::world_file &wf);
		void write_minimap_background(ww::world_file &wf);
		void write_collision_data(ww::world_file &wf);

	public:
		world_writer(editor_world *wrld);
		~world_writer();

		void write(std::string name);
	};

}
//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "world_writer.h"
#include "editor_world.h"
#include "editor_terrain.h"
#include "../framework/framework.h"
#include "../framework/logging.h"
#include "../framework/bitmap.h"
#include "../framework/colour.h"
#include "../framework/graphics.h"
#include "../framework/misc.h"
#include "../framework/exception.h"
#include "../framework/texture.h"
#include "../game/world/world_vfs.h"
#include "../game/world/terrain.h"

namespace ed {

	world_writer::world_writer(editor_world *wrld)
		: _world(wrld)
	{
	}

	world_writer::~world_writer()
	{
	}

	void world_writer::write(std::string name)
	{
		_name = name;

		ww::world_vfs vfs;
		ww::world_file wf = vfs.open_file(name, true);

		write_terrain(wf);
		write_wwmap(wf);
		write_minimap_background(wf);
		write_collision_data(wf);

		// write the screenshot as well, which is pretty simple...
		ww::world_file_entry wfe = wf.get_entry("screenshot.png");
		_world->get_screenshot().save_bitmap(wfe.get_full_path());
	}

	void world_writer::write_terrain(ww::world_file &wf)
	{
		ww::terrain *trn = _world->get_terrain();
		int version = 1;
		int trn_width = trn->get_width();
		int trn_length = trn->get_length();

		ww::world_file_entry wfe = wf.get_entry("heightfield");
		wfe.write(&version, sizeof(int));
		wfe.write(&trn_width, sizeof(int));
		wfe.write(&trn_length, sizeof(int));
		wfe.write(trn->_heights, trn_width * trn_length * sizeof(float));

		for(int patch_z = 0; patch_z < trn->get_patches_length(); patch_z++)
		{
			for(int patch_x = 0; patch_x < trn->get_patches_width(); patch_x++)
			{
				shared_ptr<fw::texture> tex = trn->get_patch_splatt(patch_x, patch_z);

				std::string name = (boost::format("splatt-%1%-%2%.png") % patch_x % patch_z).str();
				wfe = wf.get_entry(name);
				tex->save_png(wfe.get_full_path());
			}
		}
	}

	void world_writer::write_wwmap(ww::world_file &wf)
	{
		ww::world_file_entry wfe = wf.get_entry(_name + ".wwmap");
		wfe.write("<wwmap version=\"1\">");
		wfe.write((boost::format("  <description>%1%</description>") % _world->get_description()).str());
		wfe.write((boost::format("  <author>%1%</author>") % _world->get_author()).str());
		wfe.write("  <size width=\"3\" height=\"3\" />");
		wfe.write("  <players>");
		for(std::map<int, fw::vector>::iterator it = _world->get_player_starts().begin();
			it != _world->get_player_starts().end(); ++it)
		{
			wfe.write((boost::format("    <player no=\"%1%\" start=\"%2% %3%\" />")
				% it->first % it->second[0] % it->second[2]).str());
		}
		wfe.write("  </players>");
		wfe.write("</wwmap>");
	}

	// The minimap background consist of basically one pixel per vertex. We calculate the colour
	// of the pixel as a combination of the height of the terrain at that point and the texture that
	// is displayed on the terrain at that point (so "high" and "grass" would be a light green, etc)
	void world_writer::write_minimap_background(ww::world_file &wf)
	{
		ww::terrain *trn = _world->get_terrain();
		int width = trn->get_width();
		int height = trn->get_length();

		// calculate the base colours we use for the minimap (basically the "average" colour
		// of each splatt texture)
		calculate_base_minimap_colours();

		std::vector<uint32_t> pixels(width * height);
		for(int z = 0; z < height; z++)
		{
			for(int x = 0; x < width; x++)
			{
				// get the base colour of the terrain
				fw::colour col = get_terrain_colour(x, z);

				// we'll normalize the height so it's between 0.25 and 1.75
				float height = trn->get_vertex_height(x, z);
				height = (20.0f + height) / 60.0f; // -20 becomes 0 and +40 becomes 1
				height *= 2.0f;

				if (height < 0.25f)
					height = 0.25f;
				if (height > 1.75f)
					height = 1.75f;

				// adjust the base colour so that it's lighter when it's higher, darker when it's lower, etc
				col *= height;

				if (col.r < 0.0f)
					col.r = 0.0f;
				if (col.r > 1.0f)
					col.r = 1.0f;
				if (col.g < 0.0f)
					col.g = 0.0f;
				if (col.g > 1.0f)
					col.g = 1.0f;
				if (col.b < 0.0f)
					col.b = 0.0f;
				if (col.b > 1.0f)
					col.b = 1.0f;

				// make sure the alpha is 1.0
				col.a = 1.0f;

				int index = x + (z * width);
				pixels[index] = col.to_abgr();
			}
		}

		fw::bitmap img(width, height);
		img.set_pixels(pixels);

		ww::world_file_entry wfe = wf.get_entry("minimap.png");
		img.save_bitmap(wfe.get_full_path());
	}

	// gets the basic colour of the terrain at the given (x,z) location
	fw::colour world_writer::get_terrain_colour(int x, int z)
	{
		int patch_x = static_cast<int>(static_cast<float>(x) / ww::terrain::PATCH_SIZE);
		int patch_z = static_cast<int>(static_cast<float>(z) / ww::terrain::PATCH_SIZE);

		editor_terrain *trn = dynamic_cast<editor_terrain *>(_world->get_terrain());
		fw::bitmap &bmp = trn->get_splatt(patch_x, patch_z);

		// centre_u and centre_v are the texture coordinates (in the range [0..1])
		// of what the cursor is currently pointing at
		float centre_u = (x - (patch_x * ww::terrain::PATCH_SIZE)) / static_cast<float>(ww::terrain::PATCH_SIZE);
		float centre_v = (z - (patch_z * ww::terrain::PATCH_SIZE)) / static_cast<float>(ww::terrain::PATCH_SIZE);

		// centre_x and centre_y are the (x,y) corrdinates (in texture space)
		// of the splatt texture where the cursor is currently pointing.
		int centre_x = static_cast<int>(centre_u * bmp.get_width());
		int centre_y = static_cast<int>(centre_v * bmp.get_height());

		fw::colour splatt_colour = bmp.get_pixel(centre_x, centre_y);

		fw::colour final_colour(0, 0, 0);
		final_colour += _base_minimap_colours[0] * splatt_colour.a;
		final_colour += _base_minimap_colours[1] * splatt_colour.r;
		final_colour += _base_minimap_colours[2] * splatt_colour.g;
		final_colour += _base_minimap_colours[3] * splatt_colour.b;
		final_colour.a = 1.0f;

		return final_colour;
	}

	void world_writer::calculate_base_minimap_colours()
	{
		editor_terrain *trn = dynamic_cast<editor_terrain *>(_world->get_terrain());
		for(int i = 0; i < 4; i++)
		{
			shared_ptr<fw::texture> layer_texture = trn->get_layer(i);
			fw::bitmap layer_bmp(*layer_texture.get());

			// resize the bitmap to 1x1 - this will be our average colour!
			layer_bmp.resize(1, 1, 2);
			_base_minimap_colours[i] = layer_bmp.get_pixel(0, 0);
		}
	}

	void world_writer::write_collision_data(ww::world_file &wf)
	{
		editor_terrain *trn = dynamic_cast<editor_terrain *>(_world->get_terrain());
		int width = trn->get_width();
		int length = trn->get_length();

		std::vector<bool> collision_data(width * length);
		trn->build_collision_data(collision_data);

		int version = 1;

		ww::world_file_entry wfe = wf.get_entry("collision_data");
		wfe.write(&version, sizeof(int));
		wfe.write(&width, sizeof(int));
		wfe.write(&length, sizeof(int));

		BOOST_FOREACH(bool b, collision_data)
		{
			uint8_t n = static_cast<uint8_t>(b);
			wfe.write(&n, 1);
		}
	}
}

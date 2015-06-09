//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "pathing_tool.h"
#include "../windows/main_menu.h"
#include "../editor_terrain.h"
#include "../editor_world.h"
#include "../../game/world/terrain_helper.h"
#include "../../framework/gui/window.h"
#include "../../framework/framework.h"
#include "../../framework/camera.h"
#include "../../framework/scenegraph.h"
#include "../../framework/index_buffer.h"
#include "../../framework/vertex_buffer.h"
#include "../../framework/vertex_formats.h"
#include "../../framework/misc.h"
#include "../../framework/model.h"
#include "../../framework/model_reader.h"
#include "../../framework/path_find.h"

#include <CEGUIEventArgs.h>
#include <CEGUISubscriberSlot.h>
#include <CEGUIWindowManager.h>
#include <elements/CEGUIRadioButton.h>
#include <elements/CEGUICheckbox.h>

static int PATCH_SIZE = 32; // our patches are independent of the terrain patches

//-----------------------------------------------------------------------------

class pathing_tool_window : public fw::gui::window
{
private:
	ed::pathing_tool *_tool;

	bool start_click(CEGUI::EventArgs const &e);
	bool end_click(CEGUI::EventArgs const &e);
	bool simplify_changed(CEGUI::EventArgs const &e);

public:
	pathing_tool_window(ed::pathing_tool *tool);
	virtual ~pathing_tool_window();

	virtual void initialise();
	virtual void show();
};

pathing_tool_window::pathing_tool_window(ed::pathing_tool *tool)
	: fw::gui::window("ToolPathing"), _tool(tool)
{
}

pathing_tool_window::~pathing_tool_window()
{
}

void pathing_tool_window::initialise()
{
	window::initialise();

	subscribe("ToolPathing/TestStart", CEGUI::RadioButton::EventSelectStateChanged,
		CEGUI::SubscriberSlot(&pathing_tool_window::start_click, this));
	subscribe("ToolPathing/TestEnd", CEGUI::RadioButton::EventSelectStateChanged,
		CEGUI::SubscriberSlot(&pathing_tool_window::end_click, this));
	subscribe("ToolPathing/Simplified", CEGUI::Checkbox::EventCheckStateChanged,
		CEGUI::SubscriberSlot(&pathing_tool_window::simplify_changed, this));
}

void pathing_tool_window::show()
{
	window::show();

	CEGUI::Checkbox *cb = get_child<CEGUI::Checkbox>("ToolPathing/Simplified");
	cb->setSelected(true);
}

bool pathing_tool_window::start_click(CEGUI::EventArgs const &e)
{
	ed::statusbar->set_message("Set test start...");
	_tool->set_test_start();
	return true;
}

bool pathing_tool_window::end_click(CEGUI::EventArgs const &e)
{
	ed::statusbar->set_message("Set test end...");
	_tool->set_test_end();
	return true;
}

bool pathing_tool_window::simplify_changed(CEGUI::EventArgs const &e)
{
	CEGUI::Checkbox *cb = get_child<CEGUI::Checkbox>("ToolPathing/Simplified");
	_tool->set_simplify(cb->isSelected());

	return true;
}

//-----------------------------------------------------------------------------

class collision_patch
{
private:
	int _patch_x, _patch_z;
	static shared_ptr<fw::index_buffer> _ib;
	shared_ptr<fw::vertex_buffer> _vb;

public:
	void bake(std::vector<bool> &data, float *heights, int width, int length, int patch_x, int patch_z);

	void render(fw::sg::scenegraph &scenegraph, fw::matrix const &world);
};

shared_ptr<fw::index_buffer> collision_patch::_ib;
shared_ptr<fw::vertex_buffer> current_path_vb;

void collision_patch::bake(std::vector<bool> &data, float *heights, int width, int length, int patch_x, int patch_z)
{
	if (!_ib)
	{
		shared_ptr<fw::index_buffer> ib(new fw::index_buffer());
		_ib = ib;

		std::vector<uint16_t> indices;
		ww::generate_terrain_indices_wireframe(indices, PATCH_SIZE);

		_ib->create_buffer(indices.size(), D3DFMT_INDEX16);
		_ib->set_data(indices.size(), &indices[0]);
	}

	std::vector<fw::vertex::xyz_c> vertices((PATCH_SIZE + 1) * (PATCH_SIZE + 1));
	for(int z = 0; z <= PATCH_SIZE; z++)
	{
		for(int x = 0; x <= PATCH_SIZE; x++)
		{
			int ix = fw::constrain((patch_x * PATCH_SIZE) + x, width);
			int iz = fw::constrain((patch_z * PATCH_SIZE) + z, length);

			bool passable = data[(iz * width) + ix];

			int index = z * (PATCH_SIZE + 1) + x;
			vertices[index] = fw::vertex::xyz_c(
				x, heights[iz * width + ix] + 0.1f, z,
				(passable ? fw::colour(0.1f, 1.0f, 0.1f) : fw::colour(1.0f, 0.1f, 0.1f)).to_d3dcolor());
		}
	}

	shared_ptr<fw::vertex_buffer> vb(new fw::vertex_buffer());
	_vb = vb;
	_vb->create_buffer<fw::vertex::xyz_c>(vertices.size());
	_vb->set_data(vertices.size(), &vertices[0]);
}

void collision_patch::render(fw::sg::scenegraph &scenegraph, fw::matrix const &world)
{
	shared_ptr<fw::sg::node> node(new fw::sg::node());
	node->set_world_matrix(world);

	// we have to set up the scenegraph node with these manually
	node->set_vertex_buffer(_vb);
	node->set_index_buffer(_ib);
	node->set_primitive_type(D3DPT_LINELIST);

	scenegraph.add_node(node);
}

//-----------------------------------------------------------------------------

namespace ed {
	REGISTER_TOOL("pathing", pathing_tool);

	pathing_tool::pathing_tool(ww::world *wrld)
		: tool(wrld), _start_set(false), _end_set(false), _test_mode(test_none)
		, _simplify(true)
	{
		_wnd = new pathing_tool_window(this);

		fw::model_reader reader;
		_marker = reader.read(fw::installed_data_path() / "meshes/marker.wwmesh");
	}

	pathing_tool::~pathing_tool()
	{
		delete _wnd;
	}

	void pathing_tool::activate()
	{
		tool::activate();
		_wnd->show();

		int width = get_terrain()->get_width();
		int length = get_terrain()->get_length();
		_collision_data.resize(width * length);
		get_terrain()->build_collision_data(_collision_data);

		_patches.resize((width/PATCH_SIZE) * (length/PATCH_SIZE));

		shared_ptr<fw::timed_path_find> pf(new fw::timed_path_find(get_terrain()->get_width(), get_terrain()->get_length(), _collision_data));
		_path_find = pf;

		fw::input *inp = fw::framework::get_instance()->get_input();
		_keybind_tokens.push_back(
			inp->bind_key(VK_MBTNLEFT, fw::input_binding(boost::bind(&pathing_tool::on_key, this, _1, _2))));
	}

	void pathing_tool::deactivate()
	{
		tool::deactivate();
		_wnd->hide();
	}

	int get_patch_index(int patch_x, int patch_z, int patches_width, int patches_length, int *new_patch_x, int *new_patch_z)
	{
		patch_x = fw::constrain(patch_x, patches_width);
		patch_z = fw::constrain(patch_z, patches_length);

		if (new_patch_x != 0)
			*new_patch_x = patch_x;
		if (new_patch_z != 0)
			*new_patch_z = patch_z;

		return patch_z * patches_width + patch_x;
	}

	void pathing_tool::render(fw::sg::scenegraph &scenegraph)
	{
		// we want to render the patches centred on where the camera is looking
		fw::camera *camera = fw::framework::get_instance()->get_camera();
		fw::vector cam_loc = camera->get_position();
		fw::vector cam_dir = camera->get_direction();
		fw::vector location = get_terrain()->get_cursor_location(cam_loc, cam_dir);

		int centre_patch_x = (int)(location[0] / PATCH_SIZE);
		int centre_patch_z = (int)(location[2] / PATCH_SIZE);

		int patch_width = get_terrain()->get_width() / PATCH_SIZE;
		int patch_length = get_terrain()->get_length() / PATCH_SIZE;
		for(int patch_z = centre_patch_z - 1; patch_z <= centre_patch_z + 1; patch_z++)
		{
			for(int patch_x = centre_patch_x - 1; patch_x <= centre_patch_x + 1; patch_x++)
			{
				int new_patch_x, new_patch_z;
				int patch_index = get_patch_index(patch_x, patch_z, patch_width, patch_length, &new_patch_x, &new_patch_z);

				if (!_patches[patch_index])
					_patches[patch_index] = bake_patch(new_patch_x, new_patch_z);

				fw::matrix world = fw::translation(
					static_cast<float>(patch_x * PATCH_SIZE),
					0,
					static_cast<float>(patch_z * PATCH_SIZE));

				_patches[patch_index]->render(scenegraph, world);
			}
		}

		if (_start_set)
		{
			fw::matrix loc(fw::translation(_start_pos));
			_marker->set_colour(fw::colour(1, 0.1f, 1, 0.1f));
			_marker->render(scenegraph, loc);
		}

		if (_end_set)
		{
			fw::matrix loc(fw::translation(_end_pos));
			_marker->set_colour(fw::colour(1, 1, 0.1f, 0.1f));
			_marker->render(scenegraph, loc);
		}

		if (current_path_vb)
		{
			shared_ptr<fw::sg::node> sgnode(new fw::sg::node());
			sgnode->set_vertex_buffer(current_path_vb);
			sgnode->set_cast_shadows(false);
			sgnode->set_primitive_type(D3DPT_LINESTRIP);
			scenegraph.add_node(sgnode);
		}
	}

	shared_ptr<collision_patch> pathing_tool::bake_patch(int patch_x, int patch_z)
	{
		shared_ptr<collision_patch> patch(new collision_patch());
		patch->bake(_collision_data, get_terrain()->get_height_data(), get_terrain()->get_width(), get_terrain()->get_length(), patch_x, patch_z);
		return patch;
	}
	
	void pathing_tool::set_simplify(bool value)
	{
		_simplify = value;
		find_path();
	}

	void pathing_tool::set_test_start()
	{
		_test_mode = test_start;
	}

	void pathing_tool::set_test_end()
	{
		_test_mode = test_end;
	}

	void pathing_tool::stop_testing()
	{
		_test_mode = test_none;
	}

	void pathing_tool::on_key(int key, bool is_down)
	{
		if (_test_mode == test_none)
			return;

		if (key == VK_MBTNLEFT && !is_down)
		{
			if (_test_mode == test_start)
			{
				_start_pos = _terrain->get_cursor_location();
				_start_set = true;
			}
			else if (_test_mode == test_end)
			{
				_end_pos = _terrain->get_cursor_location();
				_end_set = true;
			}

			find_path();
		}
	}

	void pathing_tool::find_path()
	{
		if (!_start_set || !_end_set)
			return;

		std::vector<fw::vector> full_path;
		if (!_path_find->find(full_path, _start_pos, _end_pos))
		{
			statusbar->set_message((boost::format("No path found after %1%ms") % (_path_find->total_time*1000.0f)).str());
		}
		else
		{
			std::vector<fw::vector> path;
			_path_find->simplify_path(full_path, path);
			statusbar->set_message((
					boost::format("Path found in %1%ms, %2% nodes, %3% nodes (simplified)")
						% (_path_find->total_time*1000.0f) % full_path.size() % path.size()
				).str());

			std::vector<fw::vertex::xyz_c> buffer;

			if (_simplify)
			{
				BOOST_FOREACH(fw::vector loc, path)
				{
					float height = get_terrain()->get_vertex_height(static_cast<int>(loc[0]), static_cast<int>(loc[2]));
					buffer.push_back(fw::vertex::xyz_c(loc[0], height + 0.2f, loc[2], fw::colour(1, 0.5f, 0.5f, 1).to_d3dcolor()));
				}
			}
			else
			{
				BOOST_FOREACH(fw::vector loc, full_path)
				{
					float height = get_terrain()->get_vertex_height(static_cast<int>(loc[0]), static_cast<int>(loc[2]));
					buffer.push_back(fw::vertex::xyz_c(loc[0], height + 0.2f, loc[2], fw::colour(1, 0.5f, 0.5f, 1).to_d3dcolor()));
				}
			}

			shared_ptr<fw::vertex_buffer> vb(new fw::vertex_buffer());
			vb->create_buffer<fw::vertex::xyz_c>(buffer.size());
			vb->set_data(buffer.size(), &buffer[0]);

			current_path_vb = vb;
		}
	}
}

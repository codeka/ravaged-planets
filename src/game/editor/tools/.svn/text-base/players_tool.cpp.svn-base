//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "players_tool.h"
#include "../editor_world.h"
#include "../editor_terrain.h"
#include "../../framework/gui/window.h"
#include "../../framework/model.h"
#include "../../framework/camera.h"
#include "../../framework/input.h"
#include "../../framework/framework.h"
#include "../../framework/model_reader.h"
#include "../../framework/misc.h"

#include <CEGUIEventArgs.h>
#include <CEGUISubscriberSlot.h>
#include <CEGUIWindowManager.h>
#include <elements/CEGUIItemListbox.h>
#include <elements/CEGUICheckbox.h>

namespace fs = boost::filesystem;

class players_tool_window : public fw::gui::window
{
private:
	ed::players_tool *_tool;
	CEGUI::ItemListbox *_player_no_list;
	CEGUI::Checkbox *_enabled;

	bool selection_changed(CEGUI::EventArgs const &e);
	bool enabled_changed(CEGUI::EventArgs const &e);

public:
	players_tool_window(ed::players_tool *tool);
	virtual ~players_tool_window();

	virtual void initialise();
};

players_tool_window::players_tool_window(ed::players_tool *tool)
: fw::gui::window("ToolPlayers"), _tool(tool)
{
}

players_tool_window::~players_tool_window()
{
}

void players_tool_window::initialise()
{
	fw::gui::window::initialise();

	CEGUI::WindowManager *wndmgr = CEGUI::WindowManager::getSingletonPtr();

	_enabled = get_child<CEGUI::Checkbox>("ToolPlayers/PlayerEnabled");
	_player_no_list = get_child<CEGUI::ItemListbox>("ToolPlayers/PlayerNumber");
	for(int i = 1; i <= 8; i++)
	{
		std::string name = boost::lexical_cast<std::string>(i);
		CEGUI::ItemEntry *entry = dynamic_cast<CEGUI::ItemEntry *>(
			wndmgr->createWindow("ww/ListboxItem", ("ToolPlayers/PlayerNumber/" + name).c_str()));
		entry->setText(name.c_str());
		_player_no_list->addItem(entry);
	}

	_player_no_list->selectRange(0, 0);
	subscribe(_player_no_list, CEGUI::ItemListbox::EventSelectionChanged,
		CEGUI::SubscriberSlot(&players_tool_window::selection_changed, this));

	subscribe(_enabled, CEGUI::Checkbox::EventCheckStateChanged,
		CEGUI::SubscriberSlot(&players_tool_window::enabled_changed, this));

	// tell the tool we're currently work with player #1 - that's the default
	_tool->set_curr_player(1);
}

bool players_tool_window::selection_changed(CEGUI::EventArgs const &)
{
	if (_player_no_list->getSelectedCount() != 1)
		return false;

	CEGUI::ItemEntry *item = _player_no_list->getFirstSelectedItem();
	if (item == 0)
		return false;

	int number = _player_no_list->getItemIndex(item);
	if (number >= 0)
	{
		_tool->set_curr_player(0);
		std::map<int, fw::vector> &starts = dynamic_cast<ed::editor_world *>(_tool->get_world())->get_player_starts();
		std::map<int, fw::vector>::iterator it = starts.find(number + 1);
		if (it == starts.end())
		{
			_enabled->setSelected(false);
		}
		else
		{
			_enabled->setSelected(it != starts.end());
			fw::framework::get_instance()->get_camera()->zoom_to(it->second);
		}
		_tool->set_curr_player(number + 1);
	}
	return true;
}

bool players_tool_window::enabled_changed(CEGUI::EventArgs const &)
{
	int player_no = _tool->get_curr_player();
	if (player_no <= 0)
		return true;

	fw::vector point = _tool->get_terrain()->get_camera_lookat();
	std::map<int, fw::vector> &starts = dynamic_cast<ed::editor_world *>(_tool->get_world())->get_player_starts();

	if (_enabled->isSelected())
	{
		starts[player_no] = point;
	}
	else
	{
		starts.erase(player_no);
	}
	return true;
}

namespace ed {
	REGISTER_TOOL("players", players_tool);

	players_tool::players_tool(ww::world *wrld)
		: tool(wrld)
	{
		_wnd = new players_tool_window(this);

		fw::model_reader reader;
		_marker = reader.read(fw::installed_data_path() / "meshes/marker.wwmesh");
	}

	players_tool::~players_tool()
	{
		delete _wnd;
	}

	void players_tool::activate()
	{
		tool::activate();
		_wnd->show();

		fw::input *inp = fw::framework::get_instance()->get_input();
		_keybind_tokens.push_back(
			inp->bind_key(VK_MBTNLEFT, fw::input_binding(boost::bind(&players_tool::on_key, this, _1, _2))));
	}

	void players_tool::deactivate()
	{
		tool::deactivate();
		_wnd->hide();
	}

	void players_tool::render(fw::sg::scenegraph &scenegraph)
	{
		if (_player_no <= 0)
			return;

		std::map<int, fw::vector> &starts = dynamic_cast<ed::editor_world *>(_world)->get_player_starts();
		std::map<int, fw::vector>::iterator it = starts.find(_player_no);

		// if there's no player_no in the collection, this player isn't enabled
		if (it == starts.end())
			return;

		// otherwise, render the marker at the given location
		fw::matrix loc(fw::translation(it->second));

		_marker->set_colour(fw::colour(0.75f, 0.0f, 1.0f, 0.0f));
		_marker->render(scenegraph, loc);
	}

	void players_tool::set_curr_player(int player_no)
	{
		_player_no = player_no;
	}

	void players_tool::on_key(int key, bool is_down)
	{
		if (key == VK_MBTNLEFT && !is_down)
		{
			if (_player_no <= 0)
				return;

			std::map<int, fw::vector> &starts = dynamic_cast<ed::editor_world *>(_world)->get_player_starts();
			std::map<int, fw::vector>::iterator it = starts.find(_player_no);
			if (it == starts.end())
				return;

			it->second = _terrain->get_cursor_location();
		}
	}

}
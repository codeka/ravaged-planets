/*
#include "open_map.h"
#include "../editor_screen.h"
#include "../../framework/framework.h"
#include "../../framework/gui/cegui.h"
#include "../../game/world/world.h"
#include "../../game/world/world_reader.h"
#include "../../game/world/world_vfs.h"

#include <CEGUIWindow.h>
#include <CEGUIWindowManager.h>
#include <elements/CEGUIPushButton.h>
#include <elements/CEGUIItemListBox.h>

namespace ed {

	open_map_window *open_map = 0;

	open_map_window::open_map_window()
		: window("OpenMap")
	{
	}

	open_map_window::~open_map_window()
	{
	}

	void open_map_window::initialise()
	{
		window::initialise();

		_maps = get_child<CEGUI::ItemListbox>("OpenMap/MapList");
		CEGUI::WindowManager &wndmgr = CEGUI::WindowManager::getSingleton();

		ww::world_vfs vfs;
		std::vector<ww::world_summary> map_list = vfs.list_maps();

		_maps->resetList();
		BOOST_FOREACH(ww::world_summary &ws, map_list)
		{
			CEGUI::ItemEntry *entry = dynamic_cast<CEGUI::ItemEntry *>(
				wndmgr.createWindow("ww/ListboxItem", ("OpenMap/MapList/" + ws.get_name()).c_str()));
			entry->setText(ws.get_name().c_str());
			_maps->addItem(entry);
		}

		subscribe("OpenMap/Open", CEGUI::PushButton::EventClicked,
			CEGUI::SubscriberSlot(&open_map_window::open_clicked, this));
		subscribe("OpenMap/Cancel", CEGUI::PushButton::EventClicked,
			CEGUI::SubscriberSlot(&open_map_window::cancel_clicked, this));
	}

	bool open_map_window::open_clicked(CEGUI::EventArgs const &)
	{
		CEGUI::ItemEntry *selected = _maps->getLastSelectedItem();
		if (selected != 0)
		{
			std::string map_name = selected->getText().c_str();

			editor_screen::get_instance()->open_map(map_name);
		}

		hide();
		return true;
	}

	bool open_map_window::cancel_clicked(CEGUI::EventArgs const &)
	{
		hide();
		return true;
	}
}
*/

//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "new_map.h"
#include "message_box.h"
#include "../editor_screen.h"
#include "../../framework/framework.h"
#include "../../framework/gui/cegui.h"
#include "../../game/application.h"
#include "../../game/screens/screen.h"
#include "../../game/world/terrain.h"

#include <CEGUIWindow.h>
#include <elements/CEGUIPushButton.h>

namespace ed {

	new_map_window *new_map = 0;

	new_map_window::new_map_window()
		: fw::gui::window("NewMap")
	{
	}

	new_map_window::~new_map_window()
	{
	}

	void new_map_window::initialise()
	{
		window::initialise();

		subscribe("NewMap/OK", CEGUI::PushButton::EventClicked,
			CEGUI::SubscriberSlot(&new_map_window::ok_clicked, this));
		subscribe("NewMap/Cancel", CEGUI::PushButton::EventClicked,
			CEGUI::SubscriberSlot(&new_map_window::cancel_clicked, this));
	}

	bool new_map_window::ok_clicked(CEGUI::EventArgs const &)
	{
		hide();

		int width;
		int height;
		try
		{
			width = boost::lexical_cast<int>(get_child("NewMap/Width")->getText().c_str());
			height = boost::lexical_cast<int>(get_child("NewMap/Height")->getText().c_str());
		}
		catch(boost::bad_lexical_cast &)
		{
			message_box->show("Invalid Parameters", "Width and Height must be an integer.");
			return true;
		}

		editor_screen::get_instance()->new_map(width * ww::terrain::PATCH_SIZE, width * ww::terrain::PATCH_SIZE);
		return true;
	}

	bool new_map_window::cancel_clicked(CEGUI::EventArgs const &)
	{
		hide();
		return true;
	}
}
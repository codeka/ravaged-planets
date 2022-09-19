//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "player_properties_window.h"
#include "../../../framework/gui/color_chooser.h"
#include "../../simulation/local_player.h"

#include <CEGUIWindow.h>
#include <CEGUIEventArgs.h>
#include <CEGUISubscriberSlot.h>
#include <elements/CEGUIPushButton.h>

namespace ww {

	player_properties_window::player_properties_window()
		: window("PlayerProperties"), _color_chooser(0)
	{
	}

	player_properties_window::~player_properties_window()
	{
		delete _color_chooser;
	}

	void player_properties_window::initialise()
	{
		window::initialise();

		_color_chooser = new fw::color_chooser(get_child("PlayerProperties/ColourContainer"));
		_color_chooser->initialise(player_colors);

		subscribe("PlayerProperties/OK", CEGUI::PushButton::EventClicked,
			CEGUI::SubscriberSlot(&player_properties_window::ok_clicked, this));
		subscribe("PlayerProperties/Cancel", CEGUI::PushButton::EventClicked,
			CEGUI::SubscriberSlot(&player_properties_window::cancel_clicked, this));
	}

	bool player_properties_window::ok_clicked(CEGUI::EventArgs const &)
	{
		hide();
		return  true;
	}

	bool player_properties_window::cancel_clicked(CEGUI::EventArgs const &)
	{
		hide();
		return true;
	}

}

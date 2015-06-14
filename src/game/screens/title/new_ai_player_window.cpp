//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "new_ai_player_window.h"
#include "new_game_window.h"
#include "../../ai/ai_player.h"
#include "../../ai/ai_scriptmgr.h"
#include "../../simulation/simulation_thread.h"
#include "../../../framework/colour.h"
#include "../../../framework/gui/colour_chooser.h"
#include "../../../framework/logging.h"

#include <CEGUIWindow.h>
#include <CEGUIEventArgs.h>
#include <CEGUISubscriberSlot.h>
#include <elements/CEGUIPushButton.h>
#include <elements/CEGUICombobox.h>
#include <elements/CEGUIListboxTextItem.h>

namespace ww {

	new_ai_player_window::new_ai_player_window()
		: window("NewAiPlayer"), _colour_chooser(0)
	{
	}

	new_ai_player_window::~new_ai_player_window()
	{
		delete _colour_chooser;
	}

	void new_ai_player_window::initialise()
	{
		window::initialise();

		_colour_chooser = new fw::colour_chooser(get_child("NewAiPlayer/ColourContainer"));
		_colour_chooser->initialise(player_colours);

		subscribe("NewAiPlayer/OK", CEGUI::PushButton::EventClicked,
			CEGUI::SubscriberSlot(&new_ai_player_window::ok_clicked, this));
		subscribe("NewAiPlayer/Cancel", CEGUI::PushButton::EventClicked,
			CEGUI::SubscriberSlot(&new_ai_player_window::cancel_clicked, this));

		CEGUI::Combobox *cb = get_child<CEGUI::Combobox>("NewAiPlayer/Script");
		cb->setReadOnly(true); // make the editbox read-only

		// add each of the scripts to the "scripts" combobox so the user can choose
		// which one he wants
		ai_scriptmgr scriptmgr;
		std::vector<script_desc> &scripts = scriptmgr.get_scripts();
		BOOST_FOREACH(script_desc &desc, scripts)
		{
			cb->addItem(new CEGUI::ListboxTextItem(desc.name, 0, &desc));
		}

		// if there's at least one script (which there should be...) we'll want the default
		// one to be the first one in the list.
		if (scripts.size() > 0)
		{
			cb->setText(scripts[0].name);
		}
	}

	void new_ai_player_window::show()
	{
		window::show();

		CEGUI::Window *player_name = get_child("NewAiPlayer/Name");

		int max_player_no = 0;
		BOOST_FOREACH(player *plyr, simulation_thread::get_instance()->get_players())
		{
			int player_no = plyr->get_player_no();
			if (player_no > max_player_no)
				max_player_no = player_no;
		}

		std::stringstream ss;
		ss << "Player " << (max_player_no + 1);
		player_name->setText(CEGUI::String(ss.str().c_str()));
	}

	bool new_ai_player_window::ok_clicked(CEGUI::EventArgs const &)
	{
		CEGUI::Window *player_name = get_child("NewAiPlayer/Name");
		std::string name(player_name->getText().c_str());

		script_desc *desc = 0;
		CEGUI::Combobox *cb = get_child<CEGUI::Combobox>("NewAiPlayer/Script");
		CEGUI::ListboxItem *selected = cb->getSelectedItem();
		if (selected != 0)
		{
			desc = static_cast<script_desc *>(selected->getUserData());
		}
		else
		{
			ai_scriptmgr scriptmgr;
			std::vector<script_desc> &scripts = scriptmgr.get_scripts();
			desc = &scripts[0];
		}

		uint16_t game_id = simulation_thread::get_instance()->get_game_id();
		if (game_id == 0)
		{
			// if we're not part of a multiplayer game, just add the AI player and be
			// done with it (choose a player_id based on the number of players so far)
			int num_players = simulation_thread::get_instance()->get_players().size();

			ai_player *ply = new ai_player(name, desc, static_cast<uint8_t>(num_players + 1));
			if (!ply->is_valid_state())
			{
				new_game->add_chat_msg("Error loading player script, check error log.");
			}
			else
			{
				ply->set_colour(_colour_chooser->get_colour());
				simulation_thread::get_instance()->add_ai_player(ply);
			}
		}
		else
		{
			// if we're part of a multiplayer game, we have to join this player like any
			// other player...

			fw::debug << boost::format("you're already in game %1%, cannot add an AI player (that feature is not yet implemented)")
				% game_id << std::endl;
		}

		hide();
		return  true;
	}

	bool new_ai_player_window::cancel_clicked(CEGUI::EventArgs const &)
	{
		hide();
		return true;
	}

}
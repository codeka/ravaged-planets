//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "join_game_window.h"
#include "main_menu_window.h"
#include "new_game_window.h"
#include "../../session/session.h"
#include "../../session/session_request.h"
#include "../../../framework/logging.h"
#include "../../../framework/misc.h"
#include "../../../framework/lang.h"

#include <absl/strings/numbers.h>

#include <CEGUIWindow.h>
#include <CEGUIWindowManager.h>
#include <CEGUIUDim.h>
#include <elements/CEGUIPushButton.h>
#include <elements/CEGUIMultiColumnList.h>
#include <elements/CEGUIListboxTextItem.h>

namespace ww {

	join_game_window::join_game_window()
		: fw::gui::window("JoinGame"), sess_state_(ww::Session::disconnected)
	{
	}

	join_game_window::~join_game_window()
	{
	}

	void join_game_window::initialise()
	{
		fw::gui::window::initialise();

		_games_list = get_child<CEGUI::MultiColumnList>("JoinGame/GameList");
		_games_list->addColumn(reinterpret_cast<CEGUI::utf8 const *>(fw::text("title.join-game.col-id").c_str()), 1, CEGUI::UDim(0.24f, 0.0f));
		_games_list->addColumn(reinterpret_cast<CEGUI::utf8 const *>(fw::text("title.join-game.col-user").c_str()), 2, CEGUI::UDim(0.24f, 0.0f));
		_games_list->addColumn(reinterpret_cast<CEGUI::utf8 const *>(fw::text("title.join-game.col-game-name").c_str()), 3, CEGUI::UDim(0.24f, 0.0f));
		_games_list->addColumn(reinterpret_cast<CEGUI::utf8 const *>(fw::text("title.join-game.col-address").c_str()), 4, CEGUI::UDim(0.24f, 0.0f));

		subscribe(_games_list,CEGUI::MultiColumnList::EventSelectionChanged,
			CEGUI::SubscriberSlot(&join_game_window::games_list_selection_changed, this));

		CEGUI::Window *wnd = get_child("JoinGame/Join");
		subscribe(wnd, CEGUI::PushButton::EventClicked,
			CEGUI::SubscriberSlot(&join_game_window::join_game_clicked, this));
		wnd->setEnabled(false);

		subscribe("JoinGame/Cancel", CEGUI::PushButton::EventClicked,
			CEGUI::SubscriberSlot(&join_game_window::cancel_clicked, this));
	}

	void join_game_window::update() {
		if (Session::get_instance() == 0) {
			return;
		}

		Session::SessionState curr_state = Session::get_instance()->get_state();
		if (curr_state != sess_state_) {
			sess_state_ = curr_state;

			if (sess_state_ == Session::logged_in)
			{
				refresh_games_list();
			}
		}
	}

	void join_game_window::show() {
		fw::gui::window::show();
	}

	bool join_game_window::games_list_selection_changed(CEGUI::EventArgs const &) {
		CEGUI::Window *wnd = get_child("JoinGame/Join");
		wnd->setEnabled(_games_list->getSelectedCount() > 0);
		return true;
	}

	bool join_game_window::join_game_clicked(CEGUI::EventArgs const &) {
		CEGUI::ListboxItem *selection = _games_list->getFirstSelectedItem();
		if (selection == 0)
			return true;

		std::string id = selection->getText().c_str();
		uint64_t lobby_id;
		if (!absl::SimpleAtoi(id, &lobby_id)) {
			// handle errors
			return true;
		}

		hide();
		new_game->show();
		new_game->set_enable_multiplayer_visible(false);

		Session::get_instance()->join_game(lobby_id);
		return true;
	}

	bool join_game_window::cancel_clicked(CEGUI::EventArgs const &) {
		hide();
		main_menu->show();
		return true;
	}

	void join_game_window::refresh_games_list() {
		ww::Session::get_instance()->get_games_list(
			std::bind(&join_game_window::refresh_games_list_callback, this, _1));
	}

	void join_game_window::refresh_games_list_callback(std::vector<RemoteGame> const &games) {
		_games = games;

		_games_list->resetList();
		for (RemoteGame &game : _games) {
			add_game(game);
		}
	}

	void join_game_window::add_game(RemoteGame &g) {
		std::vector<std::string> values;
		values.push_back(std::to_string(g.id));
		values.push_back(g.owner_username);
		values.push_back(g.display_name);
		values.push_back(g.owner_address);

		fw::add_multicolumn_row(_games_list, values, 0);
	}

}

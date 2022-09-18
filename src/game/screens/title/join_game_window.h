#pragma once
/*
#include "../../../framework/gui/window.h"
#include "../../session/session.h"

namespace CEGUI {
	class MultiColumnList;
	class EventArgs;
}

namespace ww {
	class remote_game;

	// The "Join Game" window is displayed when you click Join Game on the
	// main menu. We list all the games we can find and let you join one.
	class join_game_window : public fw::gui::window
	{
	private:
		ww::session::session_state _sess_state;
		std::vector<remote_game> _games;

		CEGUI::MultiColumnList *_games_list;

		bool games_list_selection_changed(CEGUI::EventArgs const &e);
		bool join_game_clicked(CEGUI::EventArgs const &e);
		bool cancel_clicked(CEGUI::EventArgs const &e);

		void refresh_games_list();
		void refresh_games_list_callback(std::vector<remote_game> const &games);
		void add_game(remote_game &g);

	public:
		join_game_window();
		virtual ~join_game_window();

		virtual void initialize();
		virtual void show();
		virtual void update();
	};

	extern join_game_window *join_game;

}
*/

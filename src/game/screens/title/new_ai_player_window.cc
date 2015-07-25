#include <functional>

#include <framework/colour.h>
#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>
#include <framework/gui/window.h>
#include <framework/gui/widget.h>
#include <framework/logging.h>

#include <game/ai/ai_player.h>
#include <game/ai/ai_scriptmgr.h>
#include <game/screens/title/new_ai_player_window.h>
#include <game/screens/title/new_game_window.h>
#include <game/simulation/simulation_thread.h>

using namespace std::placeholders;
using namespace fw::gui;

namespace game {

new_ai_player_window::new_ai_player_window() : _wnd(nullptr) {
}

new_ai_player_window::~new_ai_player_window() {
  fw::framework::get_instance()->get_gui()->detach_widget(_wnd);
}

void new_ai_player_window::initialize(new_game_window *new_game_window) {
	_new_game_window = new_game_window;
	_wnd = builder<window>(sum(pct(50), px(-150)), sum(pct(40), px(-100)), px(300), px(200))
	    << window::background("frame") << widget::visible(false);
	fw::framework::get_instance()->get_gui()->attach_widget(_wnd);

	// add each of the scripts to the "scripts" combobox so the user can choose
	// which one he wants
	ai_scriptmgr scriptmgr;
	std::vector<script_desc> &scripts = scriptmgr.get_scripts();
	BOOST_FOREACH(script_desc &desc, scripts) {
		//cb->addItem(new CEGUI::ListboxTextItem(desc.name, 0, &desc));
	}

	// if there's at least one script (which there should be...) we'll want the default
	// one to be the first one in the list.
	if (scripts.size() > 0) {
		//cb->setText(scripts[0].name);
	}
}

void new_ai_player_window::show() {
	_wnd->set_visible(true);

	//CEGUI::Window *player_name = get_child("NewAiPlayer/Name");

	int max_player_no = 0;
	BOOST_FOREACH(player *plyr, simulation_thread::get_instance()->get_players()) {
		int player_no = plyr->get_player_no();
		if (player_no > max_player_no)
			max_player_no = player_no;
	}

	std::stringstream ss;
	ss << "Player " << (max_player_no + 1);
	//player_name->setText(CEGUI::String(ss.str().c_str()));
}

bool new_ai_player_window::on_ok_clicked(widget *w) {
	//CEGUI::Window *player_name = get_child("NewAiPlayer/Name");
	std::string name("Player 2");

	script_desc *desc = 0;
	//CEGUI::Combobox *cb = get_child<CEGUI::Combobox>("NewAiPlayer/Script");
	//CEGUI::ListboxItem *selected = cb->getSelectedItem();
	//if (selected != 0) {
	//	desc = static_cast<script_desc *>(selected->getUserData());
	//} else {
	//	ai_scriptmgr scriptmgr;
	//	std::vector<script_desc> &scripts = scriptmgr.get_scripts();
	//	desc = &scripts[0];
	//}

	uint16_t game_id = simulation_thread::get_instance()->get_game_id();
	if (game_id == 0) {
		// if we're not part of a multiplayer game, just add the AI player and be
		// done with it (choose a player_id based on the number of players so far)
		int num_players = simulation_thread::get_instance()->get_players().size();

		ai_player *ply = new ai_player(name, desc, static_cast<uint8_t>(num_players + 1));
		if (!ply->is_valid_state()) {
			_new_game_window->append_chat("Error loading player script, check error log.");
		} else {
			//ply->set_colour(_colour_chooser->get_colour());
			simulation_thread::get_instance()->add_ai_player(ply);
		}
	} else {
		// if we're part of a multiplayer game, we have to join this player like any
		// other player...

		fw::debug
		    << boost::format("you're already in game %1%, cannot add an AI player (that feature is not yet implemented)")
			  % game_id << std::endl;
	}

	_wnd->set_visible(false);
	return true;
}

bool new_ai_player_window::on_cancel_clicked(widget *w) {
	_wnd->set_visible(false);
	return true;
}

}

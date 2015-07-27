#include <functional>

#include <framework/colour.h>
#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>
#include <framework/gui/listbox.h>
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

enum ids {
  AI_LIST_ID = 347456,
};

new_ai_player_window::new_ai_player_window() : _wnd(nullptr) {
}

new_ai_player_window::~new_ai_player_window() {
  fw::framework::get_instance()->get_gui()->detach_widget(_wnd);
}

void new_ai_player_window::initialize(new_game_window *new_game_window) {
	_new_game_window = new_game_window;
	_wnd = builder<window>(sum(pct(50), px(-250)), sum(pct(40), px(-100)), px(500), px(200))
	    << window::background("frame") << widget::visible(false)
      << (builder<listbox>(px(10), px(10), px(250), sum(pct(100), px(-60)))
          << widget::id(AI_LIST_ID))
      << (builder<button>(sum(pct(100), px(-220)), sum(pct(100), px(-40)), px(100), px(30))
          << button::text("Add player") << button::click(std::bind(&new_ai_player_window::on_ok_clicked, this, _1)))
      << (builder<button>(sum(pct(100), px(-110)), sum(pct(100), px(-40)), px(100), px(30))
          << button::text("Cancel") << button::click(std::bind(&new_ai_player_window::on_cancel_clicked, this, _1)));
	fw::framework::get_instance()->get_gui()->attach_widget(_wnd);

	// add each of the scripts to the "scripts" combobox so the user can choose which one he wants
	ai_scriptmgr scriptmgr;
	std::vector<script_desc> &scripts = scriptmgr.get_scripts();
	BOOST_FOREACH(script_desc &desc, scripts) {
    _wnd->find<listbox>(AI_LIST_ID)->add_item(
        builder<label>(px(8), px(0), pct(100), px(20)) << label::text(desc.name) << widget::data(desc));
	}

	// if there's at least one script (which there should be...) we'll want the default
	// one to be the first one in the list.
	if (scripts.size() > 0) {
    _wnd->find<listbox>(AI_LIST_ID)->select_item(0);
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

	script_desc *desc = nullptr;
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
		// if we're part of a multiplayer game, we have to join this player like any other player...
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

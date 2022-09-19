#include <boost/foreach.hpp>

#include <framework/misc.h>
#include <framework/logging.h>
#include <framework/framework.h>
#include <framework/camera.h>

#include <game/simulation/player.h>
#include <game/simulation/local_player.h>
#include <game/simulation/simulation_thread.h>
#include <game/simulation/commands.h>
#include <game/session/session.h>
#include <game/world/world.h>

namespace game {

local_player::local_player() :
    _last_session_state(session::disconnected), _session_id(0) {
  // by default, our player# is 1, which is what we get if we're playing
  // a non-networked game, or if we're the host of a networked game
  _player_no = 1;

  // start off with a random color as well
  int color_index = static_cast<int>(fw::random() * player_colors.size());
  _color = player_colors[color_index];
}

local_player::~local_player() {
}

void local_player::local_player_is_ready() {
  // let all the other players know that we're ready to start
  BOOST_FOREACH(player *p, simulation_thread::get_instance()->get_players()) {
    // obviously, we don't have to tell ourselves...
    if (p == this)
      continue;

    p->local_player_is_ready();
  }

  // also mark ourselves as ready!
  _is_ready_to_start = true;
}

void local_player::world_loaded() {
  fw::Vector start_loc;

  std::map<int, fw::Vector>::iterator start_it = game::world::get_instance()->get_player_starts().find(_player_no);
  if (start_it == game::world::get_instance()->get_player_starts().end()) {
    fw::debug << boost::format("WARN: no player_start defined for player %1%, choosing random") % _player_no
        << std::endl;
    start_loc = fw::Vector(13.0f, 0, 13.0f); // <todo, random
  } else {
    start_loc = start_it->second;
  }

  // todo: we should create a "capital" or "HQ" or something building instead of "factory"
  std::shared_ptr<create_entity_command> cmd(create_command<create_entity_command>());
  cmd->template_name = "factory";
  cmd->initial_position = start_loc;
  simulation_thread::get_instance()->post_command(cmd);

  // move the camera to the start location as well.
  fw::framework::get_instance()->get_camera()->set_location(start_loc);
}

void local_player::update() {
  session::session_state curr_state = session::get_instance()->get_state();
  if (curr_state != _last_session_state) {
    switch (curr_state) {
    case session::logged_in:
      _session_id = session::get_instance()->get_session_id();
      _user_id = session::get_instance()->get_user_id();
      _user_name = session::get_instance()->get_user_name();
      break;
    }

    _last_session_state = curr_state;
    simulation_thread::get_instance()->sig_players_changed();
  }
}

}

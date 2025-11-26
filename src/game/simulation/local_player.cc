
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

LocalPlayer::LocalPlayer() :
    last_session_state_(Session::SessionState::kDisconnected), session_id_(0) {
  // by default, our player# is 1, which is what we get if we're playing
  // a non-networked game, or if we're the Host of a networked game
  player_no_ = 1;

  // start off with a Random color as well
  int color_index = static_cast<int>(fw::random() * player_colors.size());
  color_ = player_colors[color_index];
}

LocalPlayer::~LocalPlayer() {
}

void LocalPlayer::local_player_is_ready() {
  // let all the other players know that we're ready to start
  for (Player *p : SimulationThread::get_instance()->get_players()) {
    // obviously, we don't have to tell ourselves...
    if (p == this)
      continue;

    p->local_player_is_ready();
  }

  // also mark ourselves as ready!
  is_ready_to_start_ = true;
}

void LocalPlayer::world_loaded() {
  fw::Vector start_loc;

  auto start_it = game::World::get_instance()->get_player_starts().find(player_no_);
  if (start_it == game::World::get_instance()->get_player_starts().end()) {
    fw::debug << "WARN: no player_start defined for player " << player_no_ << ", choosing random"
              << std::endl;
    start_loc = fw::Vector(13.0f, 0, 13.0f); // <todo, Random
  } else {
    start_loc = start_it->second;
  }

  // todo: we should create a "capital" or "HQ" or something building instead of "factory"
  std::shared_ptr<CreateEntityCommand> cmd(create_command<CreateEntityCommand>());
  cmd->template_name = "factory";
  cmd->initial_position = start_loc;
  SimulationThread::get_instance()->post_command(cmd);

  // move the camera to the start location as well.
  fw::Framework::get_instance()->get_camera()->set_location(start_loc);
}

void LocalPlayer::update() {
  Session::SessionState curr_state = Session::get_instance()->get_state();
  if (curr_state != last_session_state_) {
    switch (curr_state) {
    case Session::SessionState::kLoggedIn:
      session_id_ = Session::get_instance()->get_session_id();
      user_id_ = Session::get_instance()->get_user_id();
      user_name_ = Session::get_instance()->get_user_name();
      break;
    }

    last_session_state_ = curr_state;
    SimulationThread::get_instance()->sig_players_changed();
  }
}

}

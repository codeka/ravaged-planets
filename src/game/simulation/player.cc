#include <game/simulation/player.h>
#include <game/simulation/commands.h>

namespace game {

std::vector<fw::colour> player_colours;

player::player() :
    _user_id(0), _player_no(0), _is_ready_to_start(0), _colour(0, 0, 0) {
  if (player_colours.size() == 0) {
    player_colours.push_back(fw::colour(0.0f, 1.0f, 1.0f)); // aqua
    player_colours.push_back(fw::colour(1.0f, 1.0f, 1.0f)); // fuchsia
    player_colours.push_back(fw::colour(1.0f, 0.0f, 0.0f)); // red
    player_colours.push_back(fw::colour(0.5f, 0.5f, 0.0f)); // olive
    player_colours.push_back(fw::colour(0.0f, 1.0f, 0.0f)); // bright green
    player_colours.push_back(fw::colour(0.0f, 0.0f, 1.0f)); // blue
    player_colours.push_back(fw::colour(0.5f, 0.0f, 0.5f)); // purple
    player_colours.push_back(fw::colour(0.0f, 0.5f, 0.5f)); // teal
  }
}

player::~player() {
}

void player::update() {
}

void player::send_chat_msg(std::string const &) {
}

void player::post_commands(std::vector<std::shared_ptr<command>> &) {
}

void player::world_loaded() {
}

std::string player::get_user_name() const {
  if (_user_name == "") {
    std::stringstream ss;
    ss << "Player " << static_cast<int>(_player_no);
    return ss.str();
  } else {
    return _user_name;
  }
}

}

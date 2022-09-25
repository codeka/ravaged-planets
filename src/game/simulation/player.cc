#include <game/simulation/player.h>
#include <game/simulation/commands.h>

namespace game {

std::vector<fw::Color> player_colors;

Player::Player() :
    user_id_(0), player_no_(0), is_ready_to_start_(0), color_(0, 0, 0) {
  if (player_colors.size() == 0) {
    player_colors.push_back(fw::Color(0.0f, 1.0f, 1.0f)); // aqua
    player_colors.push_back(fw::Color(1.0f, 1.0f, 1.0f)); // fuchsia
    player_colors.push_back(fw::Color(1.0f, 0.0f, 0.0f)); // red
    player_colors.push_back(fw::Color(0.5f, 0.5f, 0.0f)); // olive
    player_colors.push_back(fw::Color(0.0f, 1.0f, 0.0f)); // bright green
    player_colors.push_back(fw::Color(0.0f, 0.0f, 1.0f)); // blue
    player_colors.push_back(fw::Color(0.5f, 0.0f, 0.5f)); // purple
    player_colors.push_back(fw::Color(0.0f, 0.5f, 0.5f)); // teal
  }
}

Player::~Player() {
}

void Player::update() {
}

void Player::send_chat_msg(std::string const &) {
}

void Player::post_commands(std::vector<std::shared_ptr<Command>> &) {
}

void Player::world_loaded() {
}

std::string Player::get_user_name() const {
  if (user_name_ == "") {
    std::stringstream ss;
    ss << "Player " << static_cast<int>(player_no_);
    return ss.str();
  } else {
    return user_name_;
  }
}

}

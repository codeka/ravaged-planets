#pragma once

#define BOOST_BIND_NO_PLACEHOLDERS // so it doesn't auto-include _1, _2 etc.
#include <boost/signals2.hpp>

#include <game/session/session.h>

namespace fw {
class bitmap;
namespace gui {
class Window;
class Widget;
}
}

namespace game {
class WorldSummary;
class GameScreenOptions;
class MainMenuWindow;
class NewAIPlayerWindow;

// This is the "new game" window where you can choose the map, number of players and so on.
class NewGameWindow {
private:
  MainMenuWindow *main_menu_window_;
  NewAIPlayerWindow *new_ai_player_window_;
  fw::gui::Window *wnd_;
  std::shared_ptr<GameScreenOptions> game_options_;
  game::session::session_state sess_state_;
  boost::signals2::connection sig_players_changed_conn_;
  boost::signals2::connection sig_chat_conn_;
  boost::signals2::connection sig_session_state_changed_;
  std::vector<uint32_t> ready_players_;

  // Set to true when we need to refresh the player list.
  bool need_refresh_players_;

  void on_players_changed();
  void refresh_players();

  std::vector<WorldSummary> map_list_;

  bool on_start_game_clicked(fw::gui::Widget *w);
  bool on_cancel_clicked(fw::gui::Widget *w);
  void on_maps_selection_changed(int index);
  bool multiplayer_enabled_checked(fw::gui::Widget *w);
  bool on_chat_filter(std::string ch);
  bool on_new_ai_clicked(fw::gui::Widget *w);
  bool player_properties_clicked(fw::gui::Widget *w);

  void add_chat_msg(std::string const &user_name, std::string const &msg);

  void on_session_state_changed(session::session_state new_state);

  void update_selection();

  void start_game();

  game::WorldSummary const &get_selected_world_summary();
public:
  NewGameWindow();
  ~NewGameWindow();

  void initialize(MainMenuWindow *MainMenuWindow, NewAIPlayerWindow *NewAIPlayerWindow);
  void show();
  void hide();
  void update();

  // adds a message to the chat log window
  void append_chat(std::string const &msg);

  // when joining a game, the "Enable Multiplayer" checkbox needs to be removed
  void set_enable_multiplayer_visible(bool visible);

  // selects the map with the given name. This is usually done to keep our selection in sync with the Host.
  void select_map(std::string const &map_name);
};

}

#pragma once

namespace fw {
class bitmap;
namespace gui {
class window;
class widget;
}
}

namespace game {
class world_summary;
class game_screen_options;
class main_menu_window;

/** This is the "new game" window where you can choose the map, number of players and so on. */
class new_game_window {
private:
  main_menu_window *_main_menu_window;
  fw::gui::window *_wnd;
  std::shared_ptr<game_screen_options> _game_options;
//  ww::session::session_state _sess_state;
//  boost::signals2::connection _sig_players_changed_conn;
//  boost::signals2::connection _sig_chat_conn;
//  boost::signals2::connection _sig_session_state_changed;
//  std::vector<uint32_t> _ready_players;

  void on_players_changed();
  void refresh_players();

  std::vector<world_summary> _map_list;

  bool on_start_game_clicked(fw::gui::widget *w);
  bool on_cancel_clicked(fw::gui::widget *w);
  bool on_maps_selection_changed(int index);
  bool multiplayer_enabled_checked(fw::gui::widget *w);
  bool chat_send_clicked(fw::gui::widget *w);
  bool new_ai_clicked(fw::gui::widget *w);
  bool player_properties_clicked(fw::gui::widget *w);

  void add_chat_msg(std::string const &user_name, std::string const &msg);

  void on_session_state_changed(/*session::session_state new_state*/);

  // set the screenshot image
  void set_screenshot(fw::bitmap *bmp);

  void update_selection();

  void start_game();

  game::world_summary const &get_selected_world_summary();
public:
  new_game_window();
  ~new_game_window();

  void initialize(main_menu_window *main_menu_window);
  void show();
  void hide();
  void update();

  // adds a message to the chat log window
  void add_chat_msg(std::string const &msg);

  // when joining a game, the "Enable Multiplayer" checkbox needs to be removed
  void set_enable_multiplayer_visible(bool visible);

  // selects the map with the given name. This is usually done to keep our selection in sync with the host.
  void select_map(std::string const &map_name);
};

extern new_game_window *new_game;

}

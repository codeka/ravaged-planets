#pragma once

#include <framework/signals.h>
#include <framework/status.h>

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
public:
  NewGameWindow();
  ~NewGameWindow();

  void initialize(MainMenuWindow *MainMenuWindow, NewAIPlayerWindow *NewAIPlayerWindow);
  void show();
  void hide();
  void update();

  // adds a message to the chat log window
  void AppendChat(std::string const &msg);

  // when joining a game, the "Enable Multiplayer" checkbox needs to be removed
  void set_enable_multiplayer_visible(bool visible);

  // selects the map with the given name. This is usually done to keep our selection in sync with the Host.
  void SelectMap(std::string const &map_name);
private:
  MainMenuWindow* main_menu_window_;
  NewAIPlayerWindow* new_ai_player_window_;
  std::shared_ptr<fw::gui::Window> wnd_;
  std::shared_ptr<GameScreenOptions> game_options_;
  game::Session::SessionState sess_state_;
  fw::SignalConnection sig_players_changed_conn_;
  fw::SignalConnection sig_chat_conn_;
  fw::SignalConnection sig_session_state_changed_;
  std::vector<uint32_t> ready_players_;

  // Set to true when we need to refresh the player list.
  bool need_refresh_players_ = false;

  void OnPlayersChanged();
  void RefreshPlayers();

  std::vector<WorldSummary> map_list_;

  bool OnStartGameClicked(fw::gui::Widget& w);
  bool OnCancelClicked(fw::gui::Widget& w);
  void OnMapsSelectionChanged(int index);
  bool MultiplayerEnabledChecked(fw::gui::Widget& w);
  bool OnChatFilter(std::string ch);
  bool OnNewAiClicked(fw::gui::Widget& w);
  bool PlayerPropertiesClicked(fw::gui::Widget& w);

  void AddChatMsg(std::string_view user_name, std::string_view msg);

  void OnSessionStateChanged(Session::SessionState new_state);

  void UpdateSelection();

  void StartGame();

  fw::StatusOr<game::WorldSummary> GetSelectedWorldSummary();
};

}

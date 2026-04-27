#include <game/screens/title/new_game_window.h>

#include <functional>
#include <memory>
#include <string>

#include <absl/strings/str_cat.h>

#include <framework/framework.h>
#include <framework/texture.h>
#include <framework/logging.h>
#include <framework/bitmap.h>
#include <framework/lang.h>
#include <framework/misc.h>
#include <framework/gui/gui.h>
#include <framework/gui/window.h>
#include <framework/gui/builder.h>
#include <framework/gui/widget.h>
#include <framework/gui/button.h>
#include <framework/gui/label.h>
#include <framework/gui/listbox.h>
#include <framework/gui/textedit.h>
#include <framework/status.h>
#include <framework/signals.h>
#include <framework/version.h>

#include <game/application.h>
#include <game/session/session.h>
#include <game/screens/game_screen.h>
#include <game/screens/title/main_menu_window.h>
#include <game/screens/title/new_ai_player_window.h>
#include <game/screens/title/player_properties_window.h>
#include <game/simulation/local_player.h>
#include <game/simulation/simulation_thread.h>
#include <game/world/world_vfs.h>
#include <game/world/world.h>
#include <framework/gui/linear_layout.h>

using namespace std::placeholders;
using namespace fw::gui;

namespace game {

enum ids {
  MAP_LIST_ID = 743432,
  MAP_SCREENSHOT_ID,
  MAP_NAME_ID,
  MAP_SIZE_ID,
  PLAYER_LIST_ID,
  CHAT_LIST_ID,
  CHAT_TEXTEDIT_ID,
};

NewGameWindow::NewGameWindow() :
    wnd_(nullptr), main_menu_window_(nullptr), sess_state_(Session::SessionState::kDisconnected) {
}

NewGameWindow::~NewGameWindow() {
}

void NewGameWindow::initialize(
    MainMenuWindow *MainMenuWindow, NewAIPlayerWindow *NewAIPlayerWindow) {
  main_menu_window_ = MainMenuWindow;
  new_ai_player_window_ = NewAIPlayerWindow;

  wnd_ = Builder<Window>()
		  << Widget::width(LayoutParams::kMatchParent, 0.f)
		  << Widget::height(LayoutParams::kMatchParent, 0.f)
      << Widget::background("title_background")
      << Widget::visible(false)
      << (Builder<LinearLayout>()
          << Widget::width(LayoutParams::kMatchParent, 0.f)
          << Widget::height(LayoutParams::kMatchParent, 0.f)
          << Widget::margin(20.f, 20.f, 20.f, 40.f)
		      << LinearLayout::orientation(LinearLayout::Orientation::kVertical)
          << (Builder<Label>()
              << Widget::width(LayoutParams::kWrapContent, 0.f)
              << Widget::height(LayoutParams::kWrapContent, 0.f)
              << Label::background("title_heading"))
          << (Builder<Label>()
              << Widget::width(LayoutParams::kWrapContent, 0.f)
              << Widget::height(LayoutParams::kWrapContent, 0.f)
              << Widget::margin(10.f, 0.f, 20.f, 0.f)
              << Label::text(std::vformat(
                    fw::text("title.sub-title"),
                    std::make_format_args("Dean Harding", "dean@codeka.com.au"))))
          << (Builder<LinearLayout>()
              << Widget::width(LayoutParams::kMatchParent, 0.f)
              << Widget::height(LayoutParams::kFixed, 0.f)
              << LinearLayout::weight(1.0f)
              << LinearLayout::orientation(LinearLayout::Orientation::kHorizontal)
              << (Builder<LinearLayout>()
                << Widget::width(LayoutParams::kFixed, 200.f)
                << Widget::height(LayoutParams::kMatchParent, 0.f)
                << LinearLayout::orientation(LinearLayout::Orientation::kVertical)
                << (Builder<Label>()
                  << Widget::width(LayoutParams::kMatchParent, 0.f)
                  << Widget::height(LayoutParams::kWrapContent, 0.f)
                  << Label::text(fw::text("title.new-game.choose-map")))
                << (Builder<Listbox>()
                    << Widget::width(LayoutParams::kMatchParent, 0.f)
                    << Widget::height(LayoutParams::kFixed, 0.f)
                    << LinearLayout::weight(1.0f)
                    << Widget::id(MAP_LIST_ID)
									  << Widget::name("map_list")
                    << Listbox::item_selected(
                        std::bind(&NewGameWindow::OnMapsSelectionChanged, this, _1))))
              << (Builder<LinearLayout>()
                << Widget::width(LayoutParams::kFixed, 0.f)
                << Widget::height(LayoutParams::kMatchParent, 0.f)
								<< Widget::margin(0.f, 0.f, 0.f, 20.f)
                << LinearLayout::weight(1.0f)
                << LinearLayout::orientation(LinearLayout::Orientation::kVertical)
                << (Builder<LinearLayout>()
                  << Widget::width(LayoutParams::kMatchParent, 0.f)
                  << Widget::height(LayoutParams::kFixed, 240.f)
                  << Widget::margin(0.f, 0.f, 20.f, 0.f)
                  << LinearLayout::orientation(LinearLayout::Orientation::kHorizontal)
                  << (Builder<LinearLayout>()
                      << Widget::width(LayoutParams::kFixed, 0.f)
                      << Widget::height(LayoutParams::kMatchParent, 0.f)
                      << LinearLayout::weight(1.0f)
                      << LinearLayout::orientation(LinearLayout::Orientation::kVertical)
                      << Widget::margin(0.f, 20.f, 0.f, 0.f)
                      << Widget::background("frame")
                      << (Builder<Label>()
                          << Widget::width(LayoutParams::kWrapContent, 0.f)
                          << Widget::height(LayoutParams::kWrapContent, 0.f)
                          << Widget::margin(20.f, 0.f, 10.f, 20.f)
                          << Label::text("hello 1")
                          << Widget::id(MAP_NAME_ID))
                      << (Builder<Label>()
                        << Widget::width(LayoutParams::kWrapContent, 0.f)
                        << Widget::height(LayoutParams::kWrapContent, 0.f)
                        << Widget::margin(0.f, 0.f, 10.f, 20.f)
                        << Label::text("hello 2")
                        << Widget::id(MAP_SIZE_ID)))
                  << (Builder<Label>()
                      << Widget::width(LayoutParams::kFixed, 320.f)
                      << Widget::height(LayoutParams::kFixed, 240.f)
                      << Label::background("frame")
                      << Label::text("screenshot")
										  << Label::text_align(Label::Alignment::kCenter)
                      << Widget::id(MAP_SCREENSHOT_ID)))
                << (Builder<LinearLayout>()
                  << Widget::width(LayoutParams::kMatchParent, 0.f)
                  << Widget::height(LayoutParams::kFixed, 0.f)
                  << LinearLayout::weight(1.0f)
                  << LinearLayout::orientation(LinearLayout::Orientation::kHorizontal)
                  << (Builder<Widget>()
                      << Widget::width(LayoutParams::kFixed, 300.f)
                      << Widget::height(LayoutParams::kMatchParent, 0.f)
                      << Widget::margin(0.f, 20.f, 0.f, 0.f)
                      << Widget::background("frame")
                      << (Builder<Listbox>()
                          << Widget::width(LayoutParams::kMatchParent, 0.f)
                          << Widget::height(LayoutParams::kMatchParent, 0.f)
												  << Widget::margin(30.f, 10.f, 50.f, 10.f)
                          << Widget::id(PLAYER_LIST_ID))
                      << (Builder<Label>()
                          << Widget::width(LayoutParams::kWrapContent, 0.f)
                          << Widget::height(LayoutParams::kFixed, 30.f)
                          << Label::text(fw::text("title.new-game.players")))
                      << (Builder<Button>()
                          << Widget::width(LayoutParams::kFixed, 100.f)
                          << Widget::height(LayoutParams::kFixed, 30.f)
                          << Widget::margin(0.f, 10.f, 10.f, 0.f)
  												<< Widget::gravity(
                              LayoutParams::Gravity::kRight | LayoutParams::Gravity::kBottom)
                          << Button::text(fw::text("title.new-game.add-ai-player"))
                          << Button::click(std::bind(&NewGameWindow::OnNewAiClicked, this, _1)))
                      << (Builder<Button>()
                          << Widget::width(LayoutParams::kFixed, 130.f)
                          << Widget::height(LayoutParams::kFixed, 30.f)
                          << Widget::gravity(
                              LayoutParams::Gravity::kRight | LayoutParams::Gravity::kBottom)
												  << Widget::margin(0.f, 120.f, 10.f, 0.f)
                          << Button::text(fw::text("title.new-game.start-multiplayer"))))
                  << (Builder<Widget>()
                    << Widget::width(LayoutParams::kFixed, 0.f)
                    << Widget::height(LayoutParams::kMatchParent, 0.f)
                    << LinearLayout::weight(1.0f)
                    << Widget::background("frame")
                    << (Builder<Listbox>()
											  << Widget::width(LayoutParams::kMatchParent, 0.f)
                        << Widget::height(LayoutParams::kMatchParent, 0.f)
											  << Widget::margin(0.f, 0.f, 26.f, 0.f)
                        << Widget::id(CHAT_LIST_ID))
                    << (Builder<TextEdit>()
                        << Widget::width(LayoutParams::kMatchParent, 0.f)
                        << Widget::height(LayoutParams::kFixed, 26.f)
											  << Widget::gravity(
                            LayoutParams::Gravity::kLeft | LayoutParams::Gravity::kBottom)
                        << Widget::id(CHAT_TEXTEDIT_ID)
                        << TextEdit::filter(std::bind(&NewGameWindow::OnChatFilter, this, _1)))
                    ))))
          << (Builder<LinearLayout>()
              << Widget::width(LayoutParams::kMatchParent, 0.f)
              << Widget::height(LayoutParams::kWrapContent, 0.f)
						  << Widget::margin(20.f, 0.f, 0.f, 0.f)
              << LinearLayout::orientation(LinearLayout::Orientation::kHorizontal)
              << (Builder<Button>()
                  << Widget::width(LayoutParams::kFixed, 100.f)
                  << Widget::height(LayoutParams::kFixed, 30.f)
                  << Button::text(fw::text("title.new-game.login"))
                  << Widget::click(std::bind(&NewGameWindow::OnCancelClicked, this, _1)))
              << (Builder<Widget>()
                  << Widget::width(LayoutParams::kFixed, 0.f)
                  << Widget::height(LayoutParams::kMatchParent, 0.f)
                  << LinearLayout::weight(1.0f))
              << (Builder<Button>()
                  << Widget::width(LayoutParams::kFixed, 100.f)
                  << Widget::height(LayoutParams::kFixed, 30.f)
                  << Widget::margin(0.f, 10.f, 0.f, 0.f)
                  << Button::text(fw::text("cancel"))
                  << Widget::click(std::bind(&NewGameWindow::OnCancelClicked, this, _1)))
              << (Builder<Button>()
                  << Widget::width(LayoutParams::kFixed, 100.f)
                  << Widget::height(LayoutParams::kFixed, 30.f)
                  << Button::text(fw::text("title.new-game.start-game"))
                  << Widget::click(std::bind(&NewGameWindow::OnStartGameClicked, this, _1)))));
  fw::Get<Gui>().AttachWindow(wnd_);
  game_options_ = std::shared_ptr<GameScreenOptions>(new GameScreenOptions());
}

void NewGameWindow::show() {
  wnd_->set_visible(true);

  WorldVfs vfs;
  map_list_ = vfs.list_maps();
  auto listbox = wnd_->Find<Listbox>(MAP_LIST_ID);
  fw::Get<fw::Graphics>().run_on_render_thread(
    [map_list = map_list_, listbox]() {
      listbox->Clear();
      for (WorldSummary const &ws : map_list) {
        std::string title = ws.get_name();
        listbox->AddItem(
            Builder<Label>()
					      << Widget::width(LayoutParams::kMatchParent, 0.f)
                << Widget::height(LayoutParams::kWrapContent, 0.f)
					      << Widget::padding(4.f, 4.f, 4.f, 4.f)
                << Label::text(title)
                << Widget::data(ws));
      }
      
      if (!map_list.empty()) {
        listbox->SelectItem(0);
      }
    });

  sig_players_changed_conn_ = SimulationThread::get_instance()->sig_players_changed.Connect(
      std::bind(&NewGameWindow::OnPlayersChanged, this));
  sig_chat_conn_ = SimulationThread::get_instance()->sig_chat.Connect(
      std::bind(&NewGameWindow::AddChatMsg, this, _1, _2));
  sig_session_state_changed_ = Session::get_instance()->sig_state_changed.Connect(
      std::bind(&NewGameWindow::OnSessionStateChanged, this, _1));

  // call this as if the session state just changed, to make sure we're up-to-date
  OnSessionStateChanged(Session::get_instance()->get_state());
  RefreshPlayers();
}

void NewGameWindow::hide() {
  wnd_->set_visible(false);

  SimulationThread::get_instance()->sig_players_changed.Disconnect(sig_players_changed_conn_);
  SimulationThread::get_instance()->sig_chat.Disconnect(sig_chat_conn_);
  Session::get_instance()->sig_state_changed.Disconnect(sig_session_state_changed_);
}

void NewGameWindow::set_enable_multiplayer_visible(bool visible) {
//  _multiplayer_enable->SetVisible(visible);
}

void NewGameWindow::update() {
  if (Session::get_instance() == 0) {
    //fw::gui::set_text(state_msg, fw::text("title.new-game.not-logged-in"));
    return;
  }

  // check which players are now ready that weren't ready before
  int num_players = 0;
  for (auto &p : SimulationThread::get_instance()->get_players()) {
    num_players++;
    if (p->is_ready()) {
      bool was_ready_before = false;
      for (uint32_t user_id : ready_players_) {
        if (user_id == p->get_user_id()) {
          was_ready_before = true;
        }
      }

      if (!was_ready_before) {
        ready_players_.push_back(p->get_user_id());

        // if they weren't ready before, but now are, we'll have to refresh the player
        // list and also print a message to the chat window saying that they're ready
        need_refresh_players_ = true;

        std::string user_name = p->get_user_name();
        std::string msg = std::vformat(
          fw::text("title.new-game.ready-to-go"), std::make_format_args(user_name));
        AppendChat(msg);
      }
    }
  }

  if (need_refresh_players_) {
    RefreshPlayers();
    need_refresh_players_ = false;
  }

  if (static_cast<int>(ready_players_.size()) == num_players) {
    // if all players are now ready to start, we can start the game
    StartGame();
  }
}

void NewGameWindow::OnSessionStateChanged(Session::SessionState new_state) {
/*
  CEGUI::Window *state_msg = get_child("NewGame/Multiplayer/Status");
  CEGUI::Window *multiplayer_enable_checkbox = get_child("NewGame/Multiplayer/Enable");

  sess_state_ = new_state;
  switch (sess_state_) {
  case session::logging_in:
    state_msg->setVisible(true);
    multiplayer_enable_checkbox->setVisible(false);
    fw::gui::set_text(state_msg, fw::text("please-wait.logging-in"));
    break;

  case session::logged_in:
    state_msg->setVisible(false);
    multiplayer_enable_checkbox->setVisible(true);
    break;

  case session::in_error:
    state_msg->setVisible(true);
    multiplayer_enable_checkbox->setVisible(false);
    fw::gui::set_text(state_msg, fw::text("title.new-game.not-logged-in"));
    break;

  case session::disconnected:
    state_msg->setVisible(true);
    multiplayer_enable_checkbox->setVisible(false);
    fw::gui::set_text(state_msg, fw::text("title.new-game.not-logged-in"));
    break;

  case session::joining_lobby:
    state_msg->setVisible(true);
    multiplayer_enable_checkbox->setVisible(false);
    fw::gui::set_text(state_msg, fw::text("title.new-game.joining-game"));
    break;

  default:
    // unknown state??
    break;
  }
*/
}

// this is called when the list of players has changed, we set a flag (because it can happen on
// another thread) and update the players in the main update() call.
void NewGameWindow::OnPlayersChanged() {
  need_refresh_players_ = true;
}

// refreshes the list of players in the game
void NewGameWindow::RefreshPlayers() {
  auto players = SimulationThread::get_instance()->get_players();
  auto players_list = wnd_->Find<Listbox>(PLAYER_LIST_ID);
  fw::Get<fw::Graphics>().run_on_render_thread([players, players_list]() {
    players_list->Clear();
    for (auto &plyr : players) {
      int player_no = static_cast<int>(plyr->get_player_no());
      std::string ready_str = plyr->is_ready() ? fw::text("title.new-game.ready") : "";
      players_list->AddItem(Builder<Label>()
				  << Widget::width(LayoutParams::kMatchParent, 0.f)
				  << Widget::height(LayoutParams::kWrapContent, 0.f)
				  << Widget::padding(4.f, 4.f, 4.f, 4.f)
          << Widget::data(plyr)
          << Label::text(absl::StrCat(plyr->get_user_name(), " ", ready_str)));
    }
  });
}

void NewGameWindow::SelectMap(std::string const &map_name) {
/*  _selection_changing = true;
  _maps->clearAllSelections();

  CEGUI::ItemEntry *entry = _maps->findItemWithText(map_name, 0);
  if (entry != 0) {
    entry->select();
  }

  _selection_changing = false;*/
}

void NewGameWindow::OnMapsSelectionChanged(int index) {
  UpdateSelection();
}

fw::StatusOr<game::WorldSummary> NewGameWindow::GetSelectedWorldSummary() {
  auto selected_widget = wnd_->Find<Listbox>(MAP_LIST_ID)->GetSelectedItem();
  if (selected_widget == nullptr) {
    // Can happen if there are no maps installed at all.
    return fw::ErrorStatus("no selected world");
  }

  if (!selected_widget->get_data().has_value()) {
    return fw::ErrorStatus("selected world has no data");
	}

  return std::any_cast<game::WorldSummary>(selected_widget->get_data());
}

bool NewGameWindow::OnNewAiClicked(Widget &w) {
  new_ai_player_window_->show();
  return true;
}

bool NewGameWindow::PlayerPropertiesClicked(Widget &w) {
  //player_properties->show();
  return true;
}

bool NewGameWindow::OnChatFilter(std::string ch) {
  if (ch != "\r" && ch != "\n" && ch != "\r\n") {
    return true;
  }

  auto ed = wnd_->Find<TextEdit>(CHAT_TEXTEDIT_ID);
  std::string msg(fw::StripSpaces(ed->get_text()));

  AddChatMsg(Session::get_instance()->get_user_name(), msg);
  SimulationThread::get_instance()->send_chat_msg(msg);

  ed->set_text("");
  return false; // ignore the newline character
}

void NewGameWindow::AddChatMsg(std::string_view user_name, std::string_view msg) {
  AppendChat(absl::StrCat(user_name, "> ", msg));
}

void NewGameWindow::AppendChat(std::string const &msg) {
  auto chat_list = wnd_->Find<Listbox>(CHAT_LIST_ID);
  fw::Get<fw::Graphics>().run_on_render_thread([chat_list, msg] {
    chat_list->AddItem(
      Builder<Label>()
          << Widget::width(LayoutParams::kMatchParent, 0.f)
          << Widget::height(LayoutParams::kWrapContent, 0.f)
			    << Widget::padding(4.f, 4.f, 4.f, 4.f)
          << Label::text(msg));
  });
}

void NewGameWindow::UpdateSelection() {
  auto ws = GetSelectedWorldSummary();
  if (!ws.ok()) {
    return;
  }

  SimulationThread::get_instance()->set_map_name(ws->get_name());

  wnd_->Find<Label>(MAP_NAME_ID)->set_text(absl::StrCat(ws->get_name(), " by ", ws->get_author()));
  int width = ws->get_width();
  int height = ws->get_height();
  int num_players = ws->get_num_players();
  wnd_->Find<Label>(MAP_SIZE_ID)->set_text(std::vformat(fw::text("title.new-game.map-size"),
      std::make_format_args(width, height, num_players)));
  wnd_->Find<Label>(MAP_SCREENSHOT_ID)->set_background(ws->get_screenshot());
}

// when we're ready to start, we need to mark our own player as ready and then
// wait for others. Once they're ready as well, start_game() is called.
bool NewGameWindow::OnStartGameClicked(Widget &w) {
  SimulationThread::get_instance()->get_local_player()->local_player_is_ready();

  // disable the start_game button, since we don't want you clicking it twice.
  w.set_enabled(false);
  return true;
}

// once all players are ready to start, this is called to actually start the game
void NewGameWindow::StartGame() {
  auto selected_widget = wnd_->Find<Listbox>(MAP_LIST_ID)->GetSelectedItem();
  if (selected_widget == nullptr) {
    // should never happen (unless you have no maps installed at all)
    return;
  }

  game::WorldSummary const &ws = std::any_cast<game::WorldSummary const &>(selected_widget->get_data());
  game_options_->map_name = ws.get_name();

  hide();

  Application *app = dynamic_cast<Application *>(fw::Framework::get_instance()->get_app());
  ScreenStack *ss = app->get_screen_stack();
  ss->set_active_screen("game", game_options_);
}

// this is called when you check/uncheck the "Enable multiplayer" checkbox. We've got
// let the session manager know we're starting a new multiplayer game (or not).
bool NewGameWindow::MultiplayerEnabledChecked(Widget &w) {
/*  bool enabled = _multiplayer_enable->isSelected();
  if (enabled) {
    session::get_instance()->create_game();

    // set up the initial "players" list
    refresh_players();
  }
*/
  return true;
}

bool NewGameWindow::OnCancelClicked(Widget &w) {
  hide();
  main_menu_window_->show();
  return true;
}

}

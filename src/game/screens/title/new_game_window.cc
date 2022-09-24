#include <functional>
#include <memory>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <framework/framework.h>
#include <framework/exception.h>
#include <framework/misc.h>
#include <framework/texture.h>
#include <framework/logging.h>
#include <framework/bitmap.h>
#include <framework/lang.h>
#include <framework/gui/gui.h>
#include <framework/gui/window.h>
#include <framework/gui/builder.h>
#include <framework/gui/widget.h>
#include <framework/gui/button.h>
#include <framework/gui/label.h>
#include <framework/gui/listbox.h>
#include <framework/gui/textedit.h>
#include <framework/version.h>

#include <game/application.h>
#include <game/session/session.h>
#include <game/screens/game_screen.h>
#include <game/screens/title/new_game_window.h>
#include <game/screens/title/main_menu_window.h>
#include <game/screens/title/new_ai_player_window.h>
#include <game/screens/title/player_properties_window.h>
#include <game/simulation/local_player.h>
#include <game/simulation/simulation_thread.h>
#include <game/world/world_vfs.h>
#include <game/world/world.h>

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
    wnd_(nullptr), main_menu_window_(nullptr), sess_state_(game::session::disconnected), need_refresh_players_(false) {
}

NewGameWindow::~NewGameWindow() {
}

void NewGameWindow::initialize(MainMenuWindow *MainMenuWindow, NewAIPlayerWindow *NewAIPlayerWindow) {
  main_menu_window_ = MainMenuWindow;
  new_ai_player_window_ = NewAIPlayerWindow;

  wnd_ = Builder<Window>(px(0), px(0), pct(100), pct(100))
      << Window::background("title_background")
      << Widget::visible(false)
      << (Builder<Label>(px(40), px(20), px(417), px(49))
        << Label::background("title_heading"))
      << (Builder<Label>(px(40), px(70), px(500), px(16))
          << Label::text((boost::format(fw::text("title.sub-title")) % "Dean Harding" % "dean@codeka.com.au").str()))
      << (Builder<Label>(px(40), px(100), px(200), px(20))
        << Label::text(fw::text("title.new-game.choose-map")))
      << (Builder<Listbox>(px(40), px(120), px(200), sum(pct(100), px(-190)))
          << Widget::id(MAP_LIST_ID)
          << Listbox::item_selected(std::bind(&NewGameWindow::on_maps_selection_changed, this, _1)))
      << (Builder<Window>(px(250), px(100), sum(pct(100), px(-260)), sum(pct(50), px(-100)))
          << Window::background("frame")
          << (Builder<Label>(px(4), px(4), fract(OtherDimension::kHeight, 1.333f), sum(pct(100), px(-8)))
            << Label::background("frame")
            << Widget::id(MAP_SCREENSHOT_ID))
          << (Builder<Label>(
              sum(fract(MAP_SCREENSHOT_ID, OtherDimension::kWidth, 1.0f), px(8)),
              px(8),
              sum(pct(100), sum(fract(MAP_SCREENSHOT_ID, OtherDimension::kWidth, -1.0f), px(-12))),
              px(18))
            << Widget::id(MAP_NAME_ID))
          << (Builder<Label>(
              sum(fract(MAP_SCREENSHOT_ID, OtherDimension::kWidth, 1.0f), px(8)),
              px(30),
              sum(pct(100), sum(fract(MAP_SCREENSHOT_ID, OtherDimension::kWidth, -1.0f), px(-12))),
              px(18))
            << Widget::id(MAP_SIZE_ID)))
      << (Builder<Window>(px(250), sum(pct(50), px(10)), sum(pct(100), px(-260)), sum(pct(50), px(-80)))
          << Window::background("frame")
          << (Builder<Label>(px(10), px(10), sum(pct(50), px(-10)), px(20))
            << Label::text(fw::text("title.new-game.players")))
          << (Builder<Listbox>(px(10), px(30), sum(pct(50), px(-10)), sum(pct(100), px(-80)))
            << Widget::id(PLAYER_LIST_ID))
          << (Builder<Listbox>(sum(pct(50), px(10)), px(10), sum(pct(50), px(-20)), sum(pct(100), px(-40)))
            << Widget::id(CHAT_LIST_ID))
          << (Builder<TextEdit>(sum(pct(50), px(10)), sum(pct(100), px(-30)), sum(pct(50), px(-20)), px(20))
            << Widget::id(CHAT_TEXTEDIT_ID)
            << TextEdit::filter(std::bind(&NewGameWindow::on_chat_filter, this, _1)))
          << (Builder<Button>(sum(pct(50), px(-110)), sum(pct(100), px(-40)), px(110), px(30))
            << Button::text(fw::text("title.new-game.add-ai-player"))
            << Button::click(std::bind(&NewGameWindow::on_new_ai_clicked, this, _1)))
          << (Builder<Button>(sum(pct(50), px(-240)), sum(pct(100), px(-40)), px(120), px(30))
            << Button::text(fw::text("title.new-game.start-multiplayer"))))
      << (Builder<Button>(px(40), sum(pct(100), px(-60)), px(150), px(30))
          << Button::text(fw::text("title.new-game.login"))
          << Widget::click(std::bind(&NewGameWindow::on_cancel_clicked, this, _1)))
      << (Builder<Button>(sum(pct(100), px(-190)), sum(pct(100), px(-60)), px(150), px(30))
          << Button::text(fw::text("cancel"))
          << Widget::click(std::bind(&NewGameWindow::on_cancel_clicked, this, _1)))
      << (Builder<Button>(sum(pct(100), px(-350)), sum(pct(100), px(-60)), px(150), px(30))
          << Button::text(fw::text("title.new-game.start-game"))
          << Widget::click(std::bind(&NewGameWindow::on_start_game_clicked, this, _1)))
      << (Builder<Label>(sum(pct(50.0f), px(100)), sum(pct(100), px(-20)), px(500), px(16))
          << Label::text(fw::version_str));
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);
  game_options_ = std::shared_ptr<GameScreenOptions>(new GameScreenOptions());
}

void NewGameWindow::show() {
  wnd_->set_visible(true);

  world_vfs vfs;
  map_list_ = vfs.list_maps();
  for (world_summary &ws : map_list_) {
    std::string title = ws.get_name();
    wnd_->find<Listbox>(MAP_LIST_ID)->add_item(
        Builder<Label>(px(8), px(0), pct(100), px(20)) << Label::text(title) << Widget::data(ws));
  }
  if (!map_list_.empty()) {
    wnd_->find<Listbox>(MAP_LIST_ID)->select_item(0);
  }

  sig_players_changed_conn_ = simulation_thread::get_instance()->sig_players_changed.connect(
      std::bind(&NewGameWindow::on_players_changed, this));
  sig_chat_conn_ = simulation_thread::get_instance()->sig_chat.connect(
      std::bind(&NewGameWindow::add_chat_msg, this, _1, _2));
  sig_session_state_changed_ = session::get_instance()->sig_state_changed.connect(
      std::bind(&NewGameWindow::on_session_state_changed, this, _1));

  // call this as if the session state just changed, to make sure we're up-to-date
  on_session_state_changed(session::get_instance()->get_state());
  refresh_players();
}

void NewGameWindow::hide() {
  wnd_->set_visible(false);

  sig_players_changed_conn_.disconnect();
  sig_chat_conn_.disconnect();
  sig_session_state_changed_.disconnect();
}

void NewGameWindow::set_enable_multiplayer_visible(bool visible) {
//  _multiplayer_enable->setVisible(visible);
}

void NewGameWindow::update() {
  if (session::get_instance() == 0) {
    //fw::gui::set_text(state_msg, fw::text("title.new-game.not-logged-in"));
    return;
  }

  // check which players are now ready that weren't ready before
  int num_players = 0;
  for (player* p : simulation_thread::get_instance()->get_players()) {
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

        std::string msg = (boost::format(fw::text("title.new-game.ready-to-go")) % p->get_user_name()).str();
        append_chat(msg);
      }
    }
  }

  if (need_refresh_players_) {
    refresh_players();
    need_refresh_players_ = false;
  }

  if (static_cast<int>(ready_players_.size()) == num_players) {
    // if all players are now ready to start, we can start the game
    start_game();
  }
}

void NewGameWindow::on_session_state_changed(session::session_state new_state) {
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

// this is called when the list of players has changed, we set a flag (because it can happen on another thread)
// and update the players in the main update() call.
void NewGameWindow::on_players_changed() {
  need_refresh_players_ = true;
}

// refreshes the list of players in the game
void NewGameWindow::refresh_players() {
  std::vector<player *> players = simulation_thread::get_instance()->get_players();

  Listbox *players_list = wnd_->find<Listbox>(PLAYER_LIST_ID);
  players_list->clear();
  for (player *plyr : players) {
    int player_no = static_cast<int>(plyr->get_player_no());

    std::string ready_str = plyr->is_ready() ? fw::text("title.new-game.ready") : "";
    players_list->add_item(Builder<Widget>(px(0), px(0), pct(100), px(20)) << Widget::data(plyr)
        << (Builder<Label>(px(8), px(0), px(30), px(20)) << Label::text(boost::lexical_cast<std::string>(player_no)))
        << (Builder<Label>(px(30), px(0), sum(pct(100), px(-80)), px(20)) << Label::text(plyr->get_user_name()))
        << (Builder<Label>(sum(pct(100), px(-50)), px(0), px(50), px(20)) << Label::text(ready_str)));
  }
}

void NewGameWindow::select_map(std::string const &map_name) {
/*  _selection_changing = true;
  _maps->clearAllSelections();

  CEGUI::ItemEntry *entry = _maps->findItemWithText(map_name, 0);
  if (entry != 0) {
    entry->select();
  }

  _selection_changing = false;*/
}

void NewGameWindow::on_maps_selection_changed(int index) {
  update_selection();
}

game::world_summary const &NewGameWindow::get_selected_world_summary() {
  Widget *selected_widget = wnd_->find<Listbox>(MAP_LIST_ID)->get_selected_item();
  if (selected_widget == nullptr) {
    // should never happen (unless you have no maps installed at all)
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("No selected world!"));
  }

  return boost::any_cast<game::world_summary const &>(selected_widget->get_data());
}

bool NewGameWindow::on_new_ai_clicked(Widget *w) {
  new_ai_player_window_->show();
  return true;
}

bool NewGameWindow::player_properties_clicked(Widget *w) {
  //player_properties->show();
  return true;
}

bool NewGameWindow::on_chat_filter(std::string ch) {
  if (ch != "\r" && ch != "\n" && ch != "\r\n") {
    return true;
  }

  TextEdit *ed = wnd_->find<TextEdit>(CHAT_TEXTEDIT_ID);
  std::string msg = ed->get_text();
  boost::trim(msg);

  add_chat_msg(session::get_instance()->get_user_name(), msg);
  simulation_thread::get_instance()->send_chat_msg(msg);

  ed->set_text("");
  return false; // ignore the newline character
}

void NewGameWindow::add_chat_msg(std::string const &user_name, std::string const &msg) {
  append_chat(user_name + "> " + msg);
}

void NewGameWindow::append_chat(std::string const &msg) {
  Listbox *chat_list = wnd_->find<Listbox>(CHAT_LIST_ID);
  chat_list->add_item(Builder<Label>(px(8), px(0), sum(pct(100), px(-16)), px(20)) << Label::text(msg));
}

void NewGameWindow::update_selection() {
  game::world_summary const &ws = get_selected_world_summary();
  simulation_thread::get_instance()->set_map_name(ws.get_name());

  wnd_->find<Label>(MAP_NAME_ID)->set_text((boost::format("%s by %s") % ws.get_name() % ws.get_author()).str());
  wnd_->find<Label>(MAP_SIZE_ID)->set_text((boost::format(fw::text("title.new-game.map-size"))
      % ws.get_width() % ws.get_height() % ws.get_num_players()).str());
  wnd_->find<Label>(MAP_SCREENSHOT_ID)->set_background(ws.get_screenshot());
}

// when we're ready to start, we need to mark our own player as ready and then
// wait for others. Once they're ready as well, start_game() is called.
bool NewGameWindow::on_start_game_clicked(Widget *w) {
  simulation_thread::get_instance()->get_local_player()->local_player_is_ready();

  // disable the start_game button, since we don't want you clicking it twice.
  w->set_enabled(false);
  return true;
}

// once all players are ready to start, this is called to actually start the game
void NewGameWindow::start_game() {
  Widget *selected_widget = wnd_->find<Listbox>(MAP_LIST_ID)->get_selected_item();
  if (selected_widget == nullptr) {
    // should never happen (unless you have no maps installed at all)
    return;
  }

  game::world_summary const &ws = boost::any_cast<game::world_summary const &>(selected_widget->get_data());
  game_options_->map_name = ws.get_name();

  hide();

  Application *app = dynamic_cast<Application *>(fw::Framework::get_instance()->get_app());
  ScreenStack *ss = app->get_screen_stack();
  ss->set_active_screen("game", game_options_);
}

// this is called when you check/uncheck the "Enable multiplayer" checkbox. We've got
// let the session manager know we're starting a new multiplayer game (or not).
bool NewGameWindow::multiplayer_enabled_checked(Widget *w) {
/*  bool enabled = _multiplayer_enable->isSelected();
  if (enabled) {
    session::get_instance()->create_game();

    // set up the initial "players" list
    refresh_players();
  }
*/
  return true;
}

bool NewGameWindow::on_cancel_clicked(Widget *w) {
  hide();
  main_menu_window_->show();
  return true;
}

}

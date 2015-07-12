#include <functional>
#include <memory>
#include <boost/foreach.hpp>

#include <framework/framework.h>
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
#include <framework/version.h>

#include <game/application.h>
//#include "../../session/session.h"
//#include "../../simulation/simulation_thread.h"
//#include "../../simulation/local_player.h"
#include <game/world/world_vfs.h>
#include <game/world/world.h>
#include <game/screens/game_screen.h>
#include <game/screens/title/new_game_window.h>
#include <game/screens/title/main_menu_window.h>
#include <game/screens/title/new_ai_player_window.h>
#include <game/screens/title/player_properties_window.h>

using namespace std::placeholders;
using namespace fw::gui;

namespace game {

enum ids {
  MAP_LIST = 743432
};

new_game_window::new_game_window() :
    _wnd(nullptr), _main_menu_window(nullptr) {
}

new_game_window::~new_game_window() {
}

void new_game_window::initialize(main_menu_window *main_menu_window) {
  _main_menu_window = main_menu_window;
  _wnd = builder<window>(px(0), px(0), pct(100), pct(100)) << window::background("title_background")
      << widget::visible(false)
      << (builder<label>(px(40), px(20), px(417), px(49)) << label::background("title_heading"))
      << (builder<label>(px(40), px(70), px(500), px(16))
          << label::text("A game by Dean Harding (dean@codeka.com.au)"))
      << (builder<label>(px(40), px(100), px(200), px(20)) << label::text("Choose map:"))
      << (builder<listbox>(px(40), px(120), px(200), sum(pct(100), px(-190)))
          << widget::id(MAP_LIST))
      << (builder<window>(px(250), px(120), sum(pct(100), px(-260)), sum(pct(50), px(-120)))
          << window::background("frame")
          << (builder<label>(px(4), px(4), fract(fw::gui::height, 1.333f), sum(pct(100), px(-8)))
              << label::background("frame")))
      << (builder<button>(px(40), sum(pct(100), px(-60)), px(150), px(30)) << button::text("Login")
          << widget::click(std::bind(&new_game_window::cancel_clicked, this, _1)))
      << (builder<button>(sum(pct(100), px(-190)), sum(pct(100), px(-60)), px(150), px(30)) << button::text("Cancel")
          << widget::click(std::bind(&new_game_window::cancel_clicked, this, _1)))
      << (builder<button>(sum(pct(100), px(-350)), sum(pct(100), px(-60)), px(150), px(30)) << button::text("Start game")
          << widget::click(std::bind(&new_game_window::start_game_clicked, this, _1)))
      << (builder<label>(sum(pct(50.0f), px(100)), sum(pct(100), px(-20)), px(500), px(16))
          << label::text(fw::version_str));
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);
  _game_options = std::shared_ptr<game_screen_options>(new game_screen_options());
}

void new_game_window::show() {
  _wnd->set_visible(true);

  world_vfs vfs;
  _map_list = vfs.list_maps();
  BOOST_FOREACH(world_summary &ws, _map_list) {
    std::string title = ws.get_name();
    _wnd->find<listbox>(MAP_LIST)->add_item(
        builder<label>(px(8), px(0), pct(100), px(20)) << label::text(title) << widget::data(ws));
  }
  if (!_map_list.empty()) {
    _wnd->find<listbox>(MAP_LIST)->select_item(0);
  }
/*
  _sig_players_changed_conn = simulation_thread::get_instance()->sig_players_changed.connect(
      boost::bind(&new_game_window::on_players_changed, this));
  _sig_chat_conn = simulation_thread::get_instance()->sig_chat.connect(
      boost::bind(&new_game_window::add_chat_msg, this, _1, _2));
  _sig_session_state_changed = session::get_instance()->sig_state_changed.connect(
      boost::bind(&new_game_window::on_session_state_changed, this, _1));
*/
  // call this as if the session state just changed, to make sure we're up-to-date
  on_session_state_changed(/*session::get_instance()->get_state()*/);
}

void new_game_window::hide() {
  _wnd->set_visible(false);
/*
  _sig_players_changed_conn.disconnect();
  _sig_chat_conn.disconnect();
  _sig_session_state_changed.disconnect();
*/
}

void new_game_window::set_enable_multiplayer_visible(bool visible) {
//  _multiplayer_enable->setVisible(visible);
}

void new_game_window::update() {
/*
  CEGUI::Window *state_msg = get_child("NewGame/Multiplayer/Status");

  if (session::get_instance() == 0) {
    fw::gui::set_text(state_msg, fw::text("title.new-game.not-logged-in"));
    return;
  }

  // if there's no maps selected, select at least the first one (or the last
  // one that was selected)
  if (_maps->getSelectedCount() == 0) {
    _selection_changing = true;
    if (_last_selected != 0)
      _last_selected->select();
    else
      _maps->selectRange(0, 0);

    update_selection();
    _selection_changing = false;
  }

  // check which players are now ready that weren't ready before
  int num_players = 0;
  BOOST_FOREACH(player * p, simulation_thread::get_instance()->get_players())
  {
    num_players++;
    if (p->is_ready()) {
      bool was_ready_before = false;
      BOOST_FOREACH(uint32_t user_id, _ready_players)
      {
        if (user_id == p->get_user_id())
        {
          was_ready_before = true;
        }
      }

      if (!was_ready_before) {
        _ready_players.push_back(p->get_user_id());

        // if he wasn't ready before, but now is, we'll have to refresh the player
        // list and also print a message to the chat window saying that they're ready
        _refresh_players = true;

        std::string msg = (boost::format(fw::text("title.new-game.ready-to-go")) % p->get_user_name()).str();
        add_chat_msg(msg);
      }
    }
  }

  if (_refresh_players) {
    refresh_players();
    _refresh_players = false;
  }

  if (static_cast<int>(_ready_players.size()) == num_players) {
    // if all players are now ready to start, we can start the game
    start_game();
  }
*/
}

void new_game_window::on_session_state_changed(/*session::session_state new_state*/) {
/*
  CEGUI::Window *state_msg = get_child("NewGame/Multiplayer/Status");
  CEGUI::Window *multiplayer_enable_checkbox = get_child("NewGame/Multiplayer/Enable");

  _sess_state = new_state;
  switch (_sess_state) {
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

// this is called when the list of players has changed, we set a flag (because it
// can happen on another thread) and update the players in the main update() call.
void new_game_window::on_players_changed() {
//  _refresh_players = true;
}

// refreshes the list of players in the game
void new_game_window::refresh_players() {
/*  std::vector<player *> players = simulation_thread::get_instance()->get_players();

  _players_list->resetList();
  BOOST_FOREACH(player * plyr, players)
  {
    int player_no = static_cast<int>(plyr->get_player_no());

    std::vector<std::string> values;
    values.push_back(boost::lexical_cast<std::string>(player_no));
    values.push_back("o");
    values.push_back(plyr->get_user_name());
    values.push_back(plyr->is_ready() ? fw::text("title.new-game.ready") : "");

    uint32_t row_no = fw::add_multicolumn_row(_players_list, values, 0);

    // set the colour of the second column to be the player's colour
    CEGUI::ListboxTextItem *item = dynamic_cast<CEGUI::ListboxTextItem *>(_players_list->getItemAtGridReference(
        CEGUI::MCLGridRef(row_no, 1)));

    CEGUI::colour col;
    col.setARGB(plyr->get_colour().to_d3dcolor());
    item->setTextColours(col);
  }*/
}

void new_game_window::select_map(std::string const &map_name) {
/*  _selection_changing = true;
  _maps->clearAllSelections();

  CEGUI::ItemEntry *entry = _maps->findItemWithText(map_name, 0);
  if (entry != 0) {
    entry->select();
  }

  _selection_changing = false;*/
}

bool new_game_window::maps_selection_changed(widget *w) {
/*  if (_selection_changing)
    return true;

  _selection_changing = true;
  update_selection();
  _selection_changing = false;
*/
  return true;
}

bool new_game_window::new_ai_clicked(widget *w) {
 //new_ai_player->show();
  return true;
}

bool new_game_window::player_properties_clicked(widget *w) {
  //player_properties->show();
  return true;
}

bool new_game_window::chat_send_clicked(widget *w) {
 /* std::string msg = _chat_input->getText().c_str();
  boost::trim(msg);

  // add the message to our chat window
  add_chat_msg(session::get_instance()->get_user_name(), msg);

  // send chat message to other players
  simulation_thread::get_instance()->send_chat_msg(msg);
*/
  return true;
}

void new_game_window::add_chat_msg(std::string const &user_name, std::string const &msg) {
  add_chat_msg(user_name + "> " + msg);
}

void new_game_window::add_chat_msg(std::string const &msg) {
/*  int index = _chat_msgs->getItemCount() + 1;
  std::string id = boost::lexical_cast<std::string>(index);

  CEGUI::ItemEntry *entry = dynamic_cast<CEGUI::ItemEntry *>(_wndmgr->createWindow("ww/ListboxItem",
      ("NewGame/Multiplayer/ChatMessages/" + id).c_str()));
  fw::gui::set_text(entry, msg);
  _chat_msgs->addItem(entry);*/
}

void new_game_window::update_selection() {
/*  if (_maps->getSelectedCount() == 0)
    return;

  _last_selected = _maps->getLastSelectedItem();
  if (_last_selected != 0) {
    world_summary &ws = *reinterpret_cast<world_summary *>(_last_selected->getUserData());

    simulation_thread::get_instance()->set_map_name(ws.get_name());

    CEGUI::Window *description = get_child("NewGame/MapDescription");
    fw::gui::set_text(description, ws.get_description());

    CEGUI::Window *author = get_child("NewGame/MapAuthor");
    if (ws.get_author() != "")
      fw::gui::set_text(author, (boost::format("by %1%") % ws.get_author()).str());

    CEGUI::Window *map_size = get_child("NewGame/MapSize");
    fw::gui::set_text(map_size, (boost::format("%1% x %2%") % ws.get_width() % ws.get_height()).str());

    if (ws.get_screenshot() != nullptr) {
      set_screenshot(ws.get_screenshot());
    } else {
      fw::bitmap no_screenshot(fw::installed_data_path() / "images/no-screenshot.png");
      set_screenshot(&no_screenshot);
    }
  }*/
}

// sets the screenshot image, we need to create a CEGUI imageset and all that....
void new_game_window::set_screenshot(fw::bitmap *bmp) {
/*  CEGUI::ImagesetManager &imgset_mgr = CEGUI::ImagesetManager::getSingleton();

  // copy the bitmap into a texture, ready to be used by CEGUI
  shared_ptr<fw::texture> texture(new fw::texture());
  texture->create(bmp->get_width(), bmp->get_height());
  fw::blit(*bmp, *texture);

  // create a CEGUI texture from the texture
  CEGUI::Direct3D9Renderer *renderer =
      dynamic_cast<CEGUI::Direct3D9Renderer *>(CEGUI::System::getSingleton().getRenderer());
  CEGUI::Texture &cegui_texture = renderer->createTexture(texture->get_d3dtexture());

  // next, check if there's an imageset with the name "NewGameScreenshot" and destroy it
  // if there is (we'll create a new one right away)
  if (imgset_mgr.isDefined("NewGameScreenshot"))
    imgset_mgr.destroy("NewGameScreenshot");

  // then, create an imageset based on our bitmap
  CEGUI::Imageset &imgset = imgset_mgr.create("NewGameScreenshot", cegui_texture);
  imgset.defineImage("Screenshot", CEGUI::Point(0, 0),
      CEGUI::Size(static_cast<float>(bmp->get_width()), static_cast<float>(bmp->get_height())), CEGUI::Point(0, 0));

  CEGUI::Window *wnd = get_child("NewGame/MapPicture");
  wnd->setProperty("Image", "set:NewGameScreenshot image:Screenshot");*/
}

// when we're ready to start, we need to mark our own player as ready and then
// wait for others. Once they're ready as well, start_game() is called.
bool new_game_window::start_game_clicked(widget *w) {
//  simulation_thread::get_instance()->get_local_player()->local_player_is_ready();

 // _start_game->setEnabled(false);
//  _maps->setEnabled(false);
  start_game();
  return true;
}

// once all players are ready to start, this is called to actually start the game
void new_game_window::start_game() {
  widget *selected_widget = _wnd->find<listbox>(MAP_LIST)->get_selected_item();
  if (selected_widget == nullptr) {
    // should never happen (unless you have no maps installed at all)
    return;
  }

  game::world_summary const &ws = boost::any_cast<game::world_summary const &>(selected_widget->get_data());
  _game_options->map_name = ws.get_name();

  hide();

  application *app = dynamic_cast<application *>(fw::framework::get_instance()->get_app());
  screen_stack *ss = app->get_screen();
  ss->set_active_screen("game", _game_options);
}

// this is called when you check/uncheck the "Enable multiplayer" checkbox. We've got
// let the session manager know we're starting a new multiplayer game (or not).
bool new_game_window::multiplayer_enabled_checked(widget *w) {
/*  bool enabled = _multiplayer_enable->isSelected();
  if (enabled) {
    session::get_instance()->create_game();

    // set up the initial "players" list
    refresh_players();
  }
*/
  return true;
}

bool new_game_window::cancel_clicked(widget *w) {
  hide();
  _main_menu_window->show();
  return true;
}

}

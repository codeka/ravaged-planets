#include <functional>

#include <framework/color.h>
#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>
#include <framework/gui/listbox.h>
#include <framework/gui/textedit.h>
#include <framework/gui/window.h>
#include <framework/gui/widget.h>
#include <framework/lang.h>
#include <framework/logging.h>

#include <game/ai/ai_player.h>
#include <game/ai/script_manager.h>
#include <game/screens/title/new_ai_player_window.h>
#include <game/screens/title/new_game_window.h>
#include <game/simulation/simulation_thread.h>

using namespace std::placeholders;
using namespace fw::gui;

namespace game {

enum ids {
  AI_LIST_ID = 347456,
  PLAYER_NAME_ID
};

NewAIPlayerWindow::NewAIPlayerWindow() : wnd_(nullptr), new_game_window_(nullptr) {
}

NewAIPlayerWindow::~NewAIPlayerWindow() {
  fw::Framework::get_instance()->get_gui()->detach_widget(wnd_);
}

void NewAIPlayerWindow::initialize(NewGameWindow *NewGameWindow) {
  new_game_window_ = NewGameWindow;
  wnd_ = Builder<Window>(sum(pct(50), px(-250)), sum(pct(40), px(-100)), px(500), px(200))
      << Window::background("frame")
      << Widget::visible(false)
      << (Builder<Listbox>(px(10), px(10), px(250), sum(pct(100), px(-60)))
          << Widget::id(AI_LIST_ID))
      << (Builder<Label>(px(270), px(10), sum(pct(100), px(-280)), px(20))
          << Label::text(fw::text("title.new-ai-player.player-name")))
      << (Builder<TextEdit>(px(270), px(30), sum(pct(100), px(-280)), px(20))
          << Widget::id(PLAYER_NAME_ID))
      << (Builder<Button>(sum(pct(100), px(-220)), sum(pct(100), px(-40)), px(100), px(30))
          << Button::text("Add player")
          << Button::click(std::bind(&NewAIPlayerWindow::on_ok_clicked, this, _1)))
      << (Builder<Button>(sum(pct(100), px(-110)), sum(pct(100), px(-40)), px(100), px(30))
          << Button::text("Cancel")
          << Button::click(std::bind(&NewAIPlayerWindow::on_cancel_clicked, this, _1)));
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);

  // add each of the scripts to the "scripts" combobox so the user can choose which one he wants
  ScriptManager scriptmgr;
  std::vector<ScriptDesc> &scripts = scriptmgr.get_scripts();
  for (ScriptDesc &desc : scripts) {
    wnd_->find<Listbox>(AI_LIST_ID)->add_item(
        Builder<Label>(px(8), px(0), pct(100), px(20)) << Label::text(desc.name) << Widget::data(desc));
  }

  // if there's at least one script (which there should be...) we'll want the default
  // one to be the first one in the list.
  if (scripts.size() > 0) {
    wnd_->find<Listbox>(AI_LIST_ID)->select_item(0);
  }
}

void NewAIPlayerWindow::show() {
  wnd_->set_visible(true);

  int max_player_no = 0;
  for (Player *plyr : SimulationThread::get_instance()->get_players()) {
    int player_no = plyr->get_player_no();
    if (player_no > max_player_no)
      max_player_no = player_no;
  }

  std::stringstream ss;
  ss << "Player " << (max_player_no + 1);
  wnd_->find<TextEdit>(PLAYER_NAME_ID)->set_text(ss.str());
}

bool NewAIPlayerWindow::on_ok_clicked(Widget *w) {
  wnd_->find<TextEdit>(PLAYER_NAME_ID)->get_text();
  std::string name("Player 2");

  Widget *selected_item = wnd_->find<Listbox>(AI_LIST_ID)->get_selected_item();
  if (selected_item == nullptr) {
    return false;
  }
  ScriptDesc const &desc = boost::any_cast<ScriptDesc const &>(selected_item->get_data());

  uint16_t game_id = SimulationThread::get_instance()->get_game_id();
  if (game_id == 0) {
    // if we're not part of a multiplayer game, just add the AI player and be
    // done with it (choose a player_id based on the number of players so far)
    int num_players = SimulationThread::get_instance()->get_players().size();

    AIPlayer *ply = new AIPlayer(name, desc, static_cast<uint8_t>(num_players + 1));
    if (!ply->is_valid_state()) {
      new_game_window_->append_chat("Error loading player script, check error log.");
    } else {
      //ply->set_color(_color_chooser->get_color());
      SimulationThread::get_instance()->add_ai_player(ply);
    }
  } else {
    // if we're part of a multiplayer game, we have to join this player like any other player...
    fw::debug << "you're already in game " << game_id
              << ", cannot add an AI player (that feature is not yet implemented)" << std::endl;
  }

  wnd_->set_visible(false);
  return true;
}

bool NewAIPlayerWindow::on_cancel_clicked(Widget *w) {
  wnd_->set_visible(false);
  return true;
}

}

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
  fw::Get<Gui>().DetachWindow(wnd_);
}

void NewAIPlayerWindow::initialize(NewGameWindow *new_game_window) {
  new_game_window_ = new_game_window;
  wnd_ = Builder<Window>()
		  << Widget::width(LayoutParams::kFixed, 500.f)
      << Widget::height(LayoutParams::kFixed, 250.f)
		  << Window::initial_position(WindowInitialPosition::Center())
      << Widget::background("frame")
      << Widget::visible(false)
      << (Builder<Listbox>()
          << Widget::width(LayoutParams::kMatchParent, 0.f)
          << Widget::height(LayoutParams::kMatchParent, 0.f)
				  << Widget::margin(10.f, 10.f, 90.f, 10.f)
          << Widget::id(AI_LIST_ID))
      << (Builder<Label>()
          << Widget::width(LayoutParams::kFixed, 100.f)
          << Widget::height(LayoutParams::kFixed, 30.f)
				  << Widget::gravity(LayoutParams::Gravity::kLeft | LayoutParams::Gravity::kBottom)
          << Widget::margin(0.f, 0.f, 50.f, 10.f)
          << Label::text(fw::text("title.new-ai-player.player-name")))
      << (Builder<TextEdit>()
          << Widget::width(LayoutParams::kMatchParent, 0.f)
          << Widget::height(LayoutParams::kFixed, 30.f)
          << Widget::gravity(LayoutParams::Gravity::kLeft | LayoutParams::Gravity::kBottom)
          << Widget::margin(0.f, 10.f, 50.f, 120.f)
          << Widget::id(PLAYER_NAME_ID))
      << (Builder<Button>()
          << Widget::width(LayoutParams::kFixed, 100.f)
          << Widget::height(LayoutParams::kFixed, 30.f)
          << Widget::gravity(LayoutParams::Gravity::kRight | LayoutParams::Gravity::kBottom)
          << Widget::margin(10.f, 10.f, 10.f, 0.f)
          << Button::text(fw::text("title.new-ai-player.add-player"))
          << Button::click(std::bind(&NewAIPlayerWindow::on_ok_clicked, this, _1)))
      << (Builder<Button>()
          << Widget::width(LayoutParams::kFixed, 100.f)
          << Widget::height(LayoutParams::kFixed, 30.f)
          << Widget::gravity(LayoutParams::Gravity::kRight | LayoutParams::Gravity::kBottom)
          << Widget::margin(10.f, 120.f, 10.f, 0.f)
          << Button::text(fw::text("cancel"))
          << Button::click(std::bind(&NewAIPlayerWindow::on_cancel_clicked, this, _1)));

  // add each of the scripts to the "scripts" combobox so the user can choose which one he wants
  ScriptManager scriptmgr;
  std::vector<ScriptDesc> &scripts = scriptmgr.get_scripts();
  for (ScriptDesc &desc : scripts) {
    wnd_->Find<Listbox>(AI_LIST_ID)->AddItem(
        Builder<Label>()
            << Widget::width(LayoutParams::kMatchParent, 0.f)
            << Widget::height(LayoutParams::kWrapContent, 0.f)
			      << Widget::padding(4.f, 4.f, 12.f, 4.f)
            << Label::text(desc.name)
            << Widget::data(desc));
  }

  // if there's at least one script (which there should be...) we'll want the default
  // one to be the first one in the list.
  if (scripts.size() > 0) {
    wnd_->Find<Listbox>(AI_LIST_ID)->SelectItem(0);
  }

  fw::Get<Gui>().AttachWindow(wnd_);
}

void NewAIPlayerWindow::show() {
  wnd_->set_visible(true);

  int max_player_no = 0;
  for (auto plyr : SimulationThread::get_instance()->get_players()) {
    int player_no = plyr->get_player_no();
    if (player_no > max_player_no)
      max_player_no = player_no;
  }

  std::stringstream ss;
  ss << "Player " << (max_player_no + 1);
  wnd_->Find<TextEdit>(PLAYER_NAME_ID)->set_text(ss.str());
}

bool NewAIPlayerWindow::on_ok_clicked(Widget &w) {
  wnd_->Find<TextEdit>(PLAYER_NAME_ID)->get_text();
  std::string name("Player 2");

  auto selected_item = wnd_->Find<Listbox>(AI_LIST_ID)->GetSelectedItem();
  if (!selected_item) {
    return false;
  }
  ScriptDesc const &desc = std::any_cast<ScriptDesc const &>(selected_item->get_data());

  uint16_t game_id = SimulationThread::get_instance()->get_game_id();
  if (game_id == 0) {
    // if we're not part of a multiplayer game, just add the AI player and be
    // done with it (choose a player_id based on the number of players so far)
    int num_players = SimulationThread::get_instance()->get_players().size();

    auto ply = std::make_shared<AIPlayer>(name, desc, static_cast<uint8_t>(num_players + 1));
    if (!ply->is_valid_state()) {
      new_game_window_->AppendChat("Error loading player script, check error log.");
    } else {
      //ply->set_color(_color_chooser->get_color());
      SimulationThread::get_instance()->add_ai_player(ply);
    }
  } else {
    // if we're part of a multiplayer game, we have to join this player like any other player...
    LOG(WARN) << "you're already in game " << game_id
              << ", cannot add an AI player (that feature is not yet implemented)";
  }

  wnd_->set_visible(false);
  return true;
}

bool NewAIPlayerWindow::on_cancel_clicked(Widget &w) {
  wnd_->set_visible(false);
  return true;
}

}

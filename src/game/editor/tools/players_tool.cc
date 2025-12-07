
#include <functional>

#include <framework/camera.h>
#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>
#include <framework/gui/listbox.h>
#include <framework/gui/textedit.h>
#include <framework/gui/window.h>
#include <framework/input.h>
#include <framework/misc.h>
#include <framework/model.h>
#include <framework/model_manager.h>

#include <game/editor/tools/players_tool.h>
#include <game/editor/editor_world.h>
#include <game/editor/editor_terrain.h>

using namespace fw::gui;
using namespace std::placeholders;

enum widget_ids {
  NUM_PLAYERS_ID,
  PLAYER_LIST_ID
};

class PlayersToolWindow {
private:
  ed::PlayersTool &tool_;
  std::shared_ptr<Window> wnd_;

  void selection_changed(int index);
  bool num_players_updated_click(fw::gui::Widget &w);

  void refresh_player_list();
public:
  PlayersToolWindow(ed::PlayersTool &tool);
  ~PlayersToolWindow();

  void show();
  void hide();
};

PlayersToolWindow::PlayersToolWindow(ed::PlayersTool &tool) : tool_(tool) {
  wnd_ = Builder<Window>(px(10), px(30), px(100), px(200)) << Window::background("frame")
      << (Builder<Label>(px(4), px(4), sum(pct(100), px(-8)), px(18)) << Label::text("Num players:"))
      << (Builder<TextEdit>(px(4), px(26), sum(pct(100), px(-8)), px(20))
          << TextEdit::text("4") << Widget::id(NUM_PLAYERS_ID))
      << (Builder<Button>(px(4), px(54), sum(pct(100), px(-8)), px(20)) << Button::text("Update")
          << Widget::click(std::bind(&PlayersToolWindow::num_players_updated_click, this, _1)))
      << (Builder<Label>(px(4), px(80), sum(pct(100), px(-8)), px(1)) << Label::background("filler"))
      << (Builder<Listbox>(px(4), px(88), sum(pct(100), px(-8)), px(108)) << Widget::id(PLAYER_LIST_ID)
          << Listbox::item_selected(std::bind(&PlayersToolWindow::selection_changed, this, _1)));
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);
}

PlayersToolWindow::~PlayersToolWindow() {
  fw::Framework::get_instance()->get_gui()->detach_widget(wnd_);
}

void PlayersToolWindow::refresh_player_list() {
  auto lb = wnd_->Find<Listbox>(PLAYER_LIST_ID);
  lb->clear();
  for (int i = 0; i < tool_.get_world()->get_player_starts().size(); i++) {
    lb->add_item(Builder<Label>(px(4), px(0), pct(100), px(20))
        << Label::text("Player " + std::to_string(i + 1)));
  }
}

void PlayersToolWindow::show() {
  wnd_->set_visible(true);
  refresh_player_list();
}

void PlayersToolWindow::hide() {
  wnd_->set_visible(false);
}

bool PlayersToolWindow::num_players_updated_click(fw::gui::Widget &w) {
  auto te = wnd_->Find<TextEdit>(NUM_PLAYERS_ID);
  int num_players;
  if (!absl::SimpleAtoi(te->get_text(), &num_players)) {
    // TODO: handle errors?
    LOG(ERR) << "couldn't parse '" << te->get_text() << "' as a number";
    return true;
  }
  std::map<int, fw::Vector> &player_starts = tool_.get_world()->get_player_starts();
  if (player_starts.size() < num_players) {
    for (int i = player_starts.size(); i < num_players; i++) {
      player_starts[i + 1] = fw::Vector(0, 0, 0);
    }
  } else if (player_starts.size() > num_players) {
    // uh, TODO
  }
  refresh_player_list();
  return true;
}


void PlayersToolWindow::selection_changed(int index) {
  tool_.set_curr_player(index + 1);
}

namespace ed {
REGISTER_TOOL("players", PlayersTool);

PlayersTool::PlayersTool(EditorWorld *wrld) : wnd_(nullptr), player_no_(1), Tool(wrld) {
  wnd_ = std::make_unique<PlayersToolWindow>(*this);
  auto model = fw::Framework::get_instance()->get_model_manager()->get_model("marker");
  if (!model.ok()) {
    LOG(ERR) << "error loading marker: " << model.status();
  } else {
    marker_ = *model;
  }
}

PlayersTool::~PlayersTool() {}

void PlayersTool::activate() {
  Tool::activate();
  wnd_->show();

  fw::Input *inp = fw::Framework::get_instance()->get_input();
  keybind_tokens_.push_back(
      inp->bind_key("Left-Mouse", fw::InputBinding(std::bind(&PlayersTool::on_key, this, _1, _2))));
}

void PlayersTool::deactivate() {
  Tool::deactivate();
  wnd_->hide();
}
/*
void PlayersTool::render(fw::sg::Scenegraph &Scenegraph) {
  if (player_no_ <= 0)
    return;

  std::map<int, fw::Vector> &starts = world_->get_player_starts();
  std::map<int, fw::Vector>::iterator it = starts.find(player_no_);

  // if there's no player_no in the collection, this player isn't enabled
  if (it == starts.end())
    return;

  // otherwise, render the marker at the given location
  fw::Matrix loc(fw::translation(it->second));

//  marker_->set_color(fw::Color(0.75f, 0.0f, 1.0f, 0.0f));
//  marker_->render(Scenegraph, loc);
}
*/
void PlayersTool::set_curr_player(int player_no) {
  player_no_ = player_no;
}

void PlayersTool::on_key(std::string_view keyname, bool is_down) {
  if (keyname == "Left-Mouse" && !is_down) {
    if (player_no_ <= 0) {
      return;
    }

    std::map<int, fw::Vector> &starts = world_->get_player_starts();
    std::map<int, fw::Vector>::iterator it = starts.find(player_no_);
    if (it == starts.end()) {
      return;
    }

    it->second = terrain_->get_cursor_location();
  }
}

}

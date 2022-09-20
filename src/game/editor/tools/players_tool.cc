
#include <functional>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

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

namespace fs = boost::filesystem;
using namespace fw::gui;
using namespace std::placeholders;

enum widget_ids {
  NUM_PLAYERS_ID,
  PLAYER_LIST_ID
};

class players_tool_window {
private:
  ed::players_tool *_tool;
  Window *_wnd;

  void selection_changed(int index);
  bool num_players_updated_click(fw::gui::Widget *w);

  void refresh_player_list();
public:
  players_tool_window(ed::players_tool *tool);
  ~players_tool_window();

  void show();
  void hide();
};

players_tool_window::players_tool_window(ed::players_tool *tool) : _tool(tool) {
  _wnd = Builder<Window>(px(10), px(30), px(100), px(200)) << Window::background("frame")
      << (Builder<Label>(px(4), px(4), sum(pct(100), px(-8)), px(18)) << Label::text("Num players:"))
      << (Builder<TextEdit>(px(4), px(26), sum(pct(100), px(-8)), px(20))
          << TextEdit::text("4") << Widget::id(NUM_PLAYERS_ID))
      << (Builder<Button>(px(4), px(54), sum(pct(100), px(-8)), px(20)) << Button::text("Update")
          << Widget::click(std::bind(&players_tool_window::num_players_updated_click, this, _1)))
      << (Builder<Label>(px(4), px(80), sum(pct(100), px(-8)), px(1)) << Label::background("filler"))
      << (Builder<Listbox>(px(4), px(88), sum(pct(100), px(-8)), px(108)) << Widget::id(PLAYER_LIST_ID)
          << Listbox::item_selected(std::bind(&players_tool_window::selection_changed, this, _1)));
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);
}

players_tool_window::~players_tool_window() {
  fw::framework::get_instance()->get_gui()->detach_widget(_wnd);
}

void players_tool_window::refresh_player_list() {
  Listbox *lb = _wnd->find<Listbox>(PLAYER_LIST_ID);
  lb->clear();
  for (int i = 0; i < _tool->get_world()->get_player_starts().size(); i++) {
    lb->add_item(Builder<Label>(px(4), px(0), pct(100), px(20))
        << Label::text("Player " + boost::lexical_cast<std::string>(i + 1)));
  }
}

void players_tool_window::show() {
  _wnd->set_visible(true);
  refresh_player_list();
}

void players_tool_window::hide() {
  _wnd->set_visible(false);
}

bool players_tool_window::num_players_updated_click(fw::gui::Widget *w) {
  TextEdit *te = _wnd->find<TextEdit>(NUM_PLAYERS_ID);
  int num_players = boost::lexical_cast<int>(te->get_text());
  std::map<int, fw::Vector> &player_starts = _tool->get_world()->get_player_starts();
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


void players_tool_window::selection_changed(int index) {
  _tool->set_curr_player(index + 1);
}

namespace ed {
REGISTER_TOOL("players", players_tool);

players_tool::players_tool(editor_world *wrld) : _wnd(nullptr), _player_no(1), tool(wrld) {
  _wnd = new players_tool_window(this);
  _marker = fw::framework::get_instance()->get_model_manager()->get_model("marker");
}

players_tool::~players_tool() {
  delete _wnd;
}

void players_tool::activate() {
  tool::activate();
  _wnd->show();

  fw::Input *inp = fw::framework::get_instance()->get_input();
  _keybind_tokens.push_back(
      inp->bind_key("Left-Mouse", fw::InputBinding(std::bind(&players_tool::on_key, this, _1, _2))));
}

void players_tool::deactivate() {
  tool::deactivate();
  _wnd->hide();
}

void players_tool::render(fw::sg::scenegraph &scenegraph) {
  if (_player_no <= 0)
    return;

  std::map<int, fw::Vector> &starts = _world->get_player_starts();
  std::map<int, fw::Vector>::iterator it = starts.find(_player_no);

  // if there's no player_no in the collection, this player isn't enabled
  if (it == starts.end())
    return;

  // otherwise, render the marker at the given location
  fw::Matrix loc(fw::translation(it->second));

  _marker->set_color(fw::Color(0.75f, 0.0f, 1.0f, 0.0f));
  _marker->render(scenegraph, loc);
}

void players_tool::set_curr_player(int player_no) {
  _player_no = player_no;
}

void players_tool::on_key(std::string keyname, bool is_down) {
  if (keyname == "Left-Mouse" && !is_down) {
    if (_player_no <= 0) {
      return;
    }

    std::map<int, fw::Vector> &starts = _world->get_player_starts();
    std::map<int, fw::Vector>::iterator it = starts.find(_player_no);
    if (it == starts.end()) {
      return;
    }

    it->second = _terrain->get_cursor_location();
  }
}

}

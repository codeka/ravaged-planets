
#include <functional>

#include <framework/framework.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/drawable.h>
#include <framework/gui/label.h>
#include <framework/gui/widget.h>
#include <framework/gui/window.h>
#include <framework/graphics.h>
#include <framework/bitmap.h>
#include <framework/logging.h>

#include <game/editor/editor_screen.h>
#include <game/editor/editor_world.h>
#include <game/editor/tools/heightfield_tool.h>
#include <game/editor/tools/texture_tool.h>
#include <game/application.h>

#include <game/editor/windows/main_menu.h>
#include <game/editor/windows/new_map.h>
#include <game/editor/windows/open_map.h>
#include <game/editor/windows/save_map.h>

namespace ed {

using namespace fw::gui;
using namespace std::placeholders;

//-----------------------------------------------------------------------------
enum IDS {
  STATUS_MESSAGE_ID,
};

/**
 * We implement the menu item logic here, since there's only one place in the whole game that uses
 * a menu item like this.
 */
class menu_item : public button {
public:
  menu_item(gui *gui);
  virtual ~menu_item();

  void on_attached_to_parent(widget *parent);
};

menu_item::menu_item(gui *gui) : button(gui) {
}

menu_item::~menu_item() {
}

void menu_item::on_attached_to_parent(widget *parent) {
  state_drawable *bkgnd = new state_drawable();
  bkgnd->add_drawable(state_drawable::normal, _gui->get_drawable_manager()->get_drawable("menu_normal"));
  bkgnd->add_drawable(state_drawable::hover, _gui->get_drawable_manager()->get_drawable("menu_hover"));
  _background = std::shared_ptr<drawable>(bkgnd);

  _text_align = button::left;
}

//-----------------------------------------------------------------------------

main_menu_window *main_menu = nullptr;

main_menu_window::main_menu_window() : _wnd(nullptr), _file_menu(nullptr), _tool_menu(nullptr) {
}

main_menu_window::~main_menu_window() {
}

void main_menu_window::initialize() {
  _wnd = builder<window>(px(0), px(0), pct(100), px(20)) << window::background("frame") << widget::visible(false)
      << (builder<menu_item>(px(0), px(0), px(50), px(20)) << button::text("File")
          << widget::click(std::bind(&main_menu_window::file_menu_clicked, this, _1)))
      << (builder<menu_item>(px(50), px(0), px(50), px(20)) << button::text("Tool")
          << widget::click(std::bind(&main_menu_window::tool_menu_clicked, this, _1)));

  _file_menu = builder<window>(px(0), px(20), px(100), px(80))
      << window::background("frame") << widget::visible(false)
      << (builder<menu_item>(px(0), px(0), px(100), px(20)) << button::text("New")
          << widget::click(std::bind(&main_menu_window::file_new_clicked, this, _1)))
      << (builder<menu_item>(px(0), px(20), px(100), px(20)) << button::text("Open")
          << widget::click(std::bind(&main_menu_window::file_open_clicked, this, _1)))
      << (builder<menu_item>(px(0), px(40), px(100), px(20)) << button::text("Save")
          << widget::click(std::bind(&main_menu_window::file_save_clicked, this, _1)))
      << (builder<menu_item>(px(0), px(60), px(100), px(20)) << button::text("Quit")
          << widget::click(std::bind(&main_menu_window::file_quit_clicked, this, _1)));

  _tool_menu = builder<window>(px(50), px(20), px(100), px(80))
      << window::background("frame") << widget::visible(false)
      << (builder<menu_item>(px(0), px(0), px(100), px(20)) << button::text("Heightfield")
          << widget::click(std::bind(&main_menu_window::tool_clicked, this, _1, "heightfield")))
      << (builder<menu_item>(px(0), px(20), px(100), px(20)) << button::text("Texture")
          << widget::click(std::bind(&main_menu_window::tool_clicked, this, _1, "texture")))
      << (builder<menu_item>(px(0), px(40), px(100), px(20)) << button::text("Players")
          << widget::click(std::bind(&main_menu_window::tool_clicked, this, _1, "players")))
      << (builder<menu_item>(px(0), px(60), px(100), px(20)) << button::text("Pathing")
          << widget::click(std::bind(&main_menu_window::tool_clicked, this, _1, "pathing")));

  fw::framework *frmwrk = fw::framework::get_instance();
  frmwrk->get_gui()->attach_widget(_wnd);
  frmwrk->get_gui()->attach_widget(_file_menu);
  frmwrk->get_gui()->attach_widget(_tool_menu);

  frmwrk->get_gui()->sig_click.connect(std::bind(&main_menu_window::global_click_handler, this, _1, _2, _3));
}

void main_menu_window::show() {
  _wnd->set_visible(true);
}

void main_menu_window::hide() {
  _wnd->set_visible(false);
}

/**
 * This is attached to the global GUI 'click' signal. If you've clicked on a widget that's not one
 * of our menus (or you clicked on blank space) then we need to hide the menus.
 */
void main_menu_window::global_click_handler(int button, bool is_down, fw::gui::widget *w) {
  std::vector<window *> menus = {_file_menu, _tool_menu};
  BOOST_FOREACH(window *menu, menus) {
    if (is_down && menu->is_visible() && !menu->is_child(w)) {
      menu->set_visible(false);
    }

    if (!is_down && menu->is_visible()) {
      menu->set_visible(false);
    }
  }
}

bool main_menu_window::file_menu_clicked(fw::gui::widget *w) {
  _file_menu->set_visible(true);
  return true;
}

bool main_menu_window::tool_menu_clicked(fw::gui::widget *w) {
  _tool_menu->set_visible(true);
  return true;
}

// when they click "File->New", we just show the "new map" window, which'll
// actually create the new map (assuming they click "OK" and that)
bool main_menu_window::file_new_clicked(fw::gui::widget *w) {
  new_map->show();
  return true;
}

bool main_menu_window::file_save_clicked(fw::gui::widget *w) {
  save_map->show();
  return true;
}

bool main_menu_window::file_open_clicked(fw::gui::widget *w) {
  open_map->show();
  return true;
}

bool main_menu_window::file_quit_clicked(fw::gui::widget *w) {
  // we don't actually "exit" the whole application, just go back to the title screen
  game::application *app = dynamic_cast<game::application *>(fw::framework::get_instance()->get_app());
  game::screen_stack *ss = app->get_screen();
  ss->set_active_screen("title");
  return true;
}

void main_menu_window::map_screenshot_clicked_finished(std::shared_ptr<fw::bitmap> bmp) {
/*  // re-show the UI, we don't need to hide it anymore!
  fw::framework::get_instance()->get_gui()->get_root_window()->setVisible(true);

  // create a "thumbnail" version, 160x120
  fw::bitmap thumbnail(bmp);
  thumbnail.resize(160, 120, 3);

  editor_world *world = dynamic_cast<editor_world *>(ww::world::get_instance());
  world->set_screenshot(thumbnail);*/
}

// We want to take a screenshot of the map at the current camera view to display
// in the "New Game" and "Join Game" windows
bool main_menu_window::map_screenshot_clicked(fw::gui::widget *w) {
  //fw::framework::get_instance()->get_gui()->get_root_window()->setVisible(false);

  // we take the screenshot in 1024x768 because the maps expect a 4:3 image. We
  // then resize it ourselves to ensure it's nice & anti-aliased, etc.
  fw::framework::get_instance()->take_screenshot(1024, 768,
      std::bind(&main_menu_window::map_screenshot_clicked_finished, this, std::placeholders::_1));

  return true;
}

// This is called when you click one of the "Tool" menu items. We figure out which
// one you clicked on and switch to that tool as appropriate.
bool main_menu_window::tool_clicked(fw::gui::widget *w, std::string tool_name) {
  editor_screen::get_instance()->set_active_tool(tool_name);
  return true;
}

//-------------------------------------------------------------------------
statusbar_window *statusbar = nullptr;

statusbar_window::statusbar_window() : _wnd(nullptr) {
}

statusbar_window::~statusbar_window() {
}

void statusbar_window::initialize() {
  _wnd = builder<window>(px(0), sum(pct(100), px(-20)), pct(100), px(20)) << window::background("frame")
      << widget::visible(false)
      << (builder<label>(px(0), px(0), pct(100), pct(100)) << widget::id(STATUS_MESSAGE_ID));
  fw::framework *frmwrk = fw::framework::get_instance();
  frmwrk->get_gui()->attach_widget(_wnd);
}

void statusbar_window::show() {
  _wnd->set_visible(true);
}

void statusbar_window::hide() {
  _wnd->set_visible(false);
}

void statusbar_window::set_message(std::string const &msg) {
  _wnd->find<label>(STATUS_MESSAGE_ID)->set_text(msg);
}

}

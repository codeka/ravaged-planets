
#include <functional>

#include <framework/framework.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/drawable.h>
#include <framework/gui/widget.h>
#include <framework/gui/window.h>
#include <framework/graphics.h>
#include <framework/bitmap.h>

#include <game/editor/editor_screen.h>
#include <game/editor/editor_world.h>
#include <game/editor/tools/heightfield_tool.h>
#include <game/editor/tools/texture_tool.h>
#include <game/application.h>

#include <game/editor/windows/main_menu.h>

namespace ed {

using namespace fw::gui;
using namespace std::placeholders;

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

main_menu_window::main_menu_window() : _wnd(nullptr), _file_menu(nullptr) {
}

main_menu_window::~main_menu_window() {
}

void main_menu_window::initialize() {
  _wnd = builder<window>(px(0), px(0), pct(100), px(20)) << window::background("frame")
      << (builder<menu_item>(px(0), px(0), px(50), px(20)) << button::text("File")
          << widget::click(std::bind(&main_menu_window::file_clicked, this, _1)))
      << (builder<menu_item>(px(50), px(0), px(50), px(20)) << button::text("Tool"));

  _file_menu = builder<window>(px(0), px(20), px(100), px(80))
      << window::background("frame") << widget::visible(false)
      << (builder<menu_item>(px(0), px(0), px(100), px(20)) << button::text("New"))
      << (builder<menu_item>(px(0), px(20), px(100), px(20)) << button::text("Open"))
      << (builder<menu_item>(px(0), px(40), px(100), px(20)) << button::text("Save"))
      << (builder<menu_item>(px(0), px(60), px(100), px(20)) << button::text("Quit"));
  fw::framework *frmwrk = fw::framework::get_instance();
  frmwrk->get_gui()->attach_widget(_wnd);
  frmwrk->get_gui()->attach_widget(_file_menu);

  frmwrk->get_gui()->sig_click.connect(std::bind(&main_menu_window::global_click_handler, this, _1, _2, _3));
/*
  subscribe("TopMenu/File/New", CEGUI::MenuItem::EventClicked,
      CEGUI::SubscriberSlot(&main_menu_window::file_new_clicked, this));
  subscribe("TopMenu/File/Save", CEGUI::MenuItem::EventClicked,
      CEGUI::SubscriberSlot(&main_menu_window::file_save_clicked, this));
  subscribe("TopMenu/File/Open", CEGUI::MenuItem::EventClicked,
      CEGUI::SubscriberSlot(&main_menu_window::file_open_clicked, this));
  subscribe("TopMenu/File/Exit", CEGUI::MenuItem::EventClicked,
      CEGUI::SubscriberSlot(&main_menu_window::file_quit_clicked, this));
  subscribe("TopMenu/Map/Screenshot", CEGUI::MenuItem::EventClicked,
      CEGUI::SubscriberSlot(&main_menu_window::map_screenshot_clicked, this));

  CEGUI::Window *wnd;

  wnd = get_child("TopMenu/Tool/Terrain");
  wnd->setUserString("tool", "heightfield");
  subscribe(wnd, CEGUI::MenuItem::EventClicked,
      CEGUI::SubscriberSlot(&main_menu_window::tool_clicked, this));

  wnd = get_child("TopMenu/Tool/TerrainTexture");
  wnd->setUserString("tool", "texture");
  subscribe(wnd, CEGUI::MenuItem::EventClicked,
      CEGUI::SubscriberSlot(&main_menu_window::tool_clicked, this));

  wnd = get_child("TopMenu/Tool/Players");
  wnd->setUserString("tool", "players");
  subscribe(wnd, CEGUI::MenuItem::EventClicked,
      CEGUI::SubscriberSlot(&main_menu_window::tool_clicked, this));

  wnd = get_child("TopMenu/Tool/Pathing");
  wnd->setUserString("tool", "pathing");
  subscribe(wnd, CEGUI::MenuItem::EventClicked,
      CEGUI::SubscriberSlot(&main_menu_window::tool_clicked, this));*/
}

/**
 * This is attached to the global GUI 'click' signal. If you've clicked on a widget that's not one
 * of our menus (or you clicked on blank space) then we need to hide the menus.
 */
void main_menu_window::global_click_handler(int button, bool is_down, fw::gui::widget *w) {
  _file_menu->set_visible(false);
}

bool main_menu_window::file_clicked(fw::gui::widget *w) {
  _file_menu->set_visible(true);
  return true;
}

// when they click "File->New", we just show the "new map" window, which'll
// actually create the new map (assuming they click "OK" and that)
bool main_menu_window::file_new_clicked(fw::gui::widget *w) {
  //new_map->show();
  return true;
}

bool main_menu_window::file_save_clicked(fw::gui::widget *w) {
 // save_map->show();
  return true;
}

bool main_menu_window::file_open_clicked(fw::gui::widget *w) {
 // open_map->show();
  return true;
}

bool main_menu_window::file_quit_clicked(fw::gui::widget *w) {
  // we don't actually "exit" the whole application, just go back to the title screen
  rp::application *app = dynamic_cast<rp::application *>(fw::framework::get_instance()->get_app());
  rp::screen_stack *ss = app->get_screen();
  ss->set_active_screen("title");
  return true;
}

void main_menu_window::map_screenshot_clicked_finished(fw::bitmap const &bmp) {
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
bool main_menu_window::tool_clicked(fw::gui::widget *w) {
//  std::string name = wnd->getUserString("tool").c_str();
//  editor_screen::get_instance()->set_active_tool(name);

  return true;
}

//-------------------------------------------------------------------------
/*
statusbar_window *statusbar = 0;

statusbar_window::statusbar_window() :
    fw::gui::window("Statusbar") {
}

statusbar_window::~statusbar_window() {
}

void statusbar_window::initialise() {
  fw::gui::window::initialise();
}

void statusbar_window::set_message(std::string const &msg) {
  fw::gui::set_text(get_child("Statusbar/Message"), msg);
}
*/
}

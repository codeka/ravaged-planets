
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
class menu_item : public Button {
public:
  menu_item(Gui *gui);
  virtual ~menu_item();

  void on_attached_to_parent(Widget *parent);
};

menu_item::menu_item(Gui *gui) : Button(gui) {
}

menu_item::~menu_item() {
}

void menu_item::on_attached_to_parent(Widget *parent) {
  StateDrawable *bkgnd = new StateDrawable();
  bkgnd->add_drawable(StateDrawable::kNormal, gui_->get_drawable_manager()->get_drawable("menu_normal"));
  bkgnd->add_drawable(StateDrawable::kHover, gui_->get_drawable_manager()->get_drawable("menu_hover"));
  background_ = std::shared_ptr<Drawable>(bkgnd);

  text_align_ = Button::kLeft;
}

//-----------------------------------------------------------------------------

MainMenuWindow *main_menu = nullptr;

MainMenuWindow::MainMenuWindow() : wnd_(nullptr), file_menu_(nullptr), tool_menu_(nullptr) {
}

MainMenuWindow::~MainMenuWindow() {
}

void MainMenuWindow::initialize() {
  wnd_ = Builder<Window>(px(0), px(0), pct(100), px(20)) << Window::background("frame") << Widget::visible(false)
      << (Builder<menu_item>(px(0), px(0), px(50), px(20)) << Button::text("File")
          << Widget::click(std::bind(&MainMenuWindow::file_menu_clicked, this, _1)))
      << (Builder<menu_item>(px(50), px(0), px(50), px(20)) << Button::text("Tool")
          << Widget::click(std::bind(&MainMenuWindow::tool_menu_clicked, this, _1)));

  file_menu_ = Builder<Window>(px(0), px(20), px(100), px(80))
      << Window::background("frame") << Widget::visible(false)
      << (Builder<menu_item>(px(0), px(0), px(100), px(20)) << Button::text("New")
          << Widget::click(std::bind(&MainMenuWindow::file_new_clicked, this, _1)))
      << (Builder<menu_item>(px(0), px(20), px(100), px(20)) << Button::text("Open")
          << Widget::click(std::bind(&MainMenuWindow::file_open_clicked, this, _1)))
      << (Builder<menu_item>(px(0), px(40), px(100), px(20)) << Button::text("Save")
          << Widget::click(std::bind(&MainMenuWindow::file_save_clicked, this, _1)))
      << (Builder<menu_item>(px(0), px(60), px(100), px(20)) << Button::text("Quit")
          << Widget::click(std::bind(&MainMenuWindow::file_quit_clicked, this, _1)));

  tool_menu_ = Builder<Window>(px(50), px(20), px(100), px(80))
      << Window::background("frame") << Widget::visible(false)
      << (Builder<menu_item>(px(0), px(0), px(100), px(20)) << Button::text("Heightfield")
          << Widget::click(std::bind(&MainMenuWindow::tool_clicked, this, _1, "heightfield")))
      << (Builder<menu_item>(px(0), px(20), px(100), px(20)) << Button::text("Texture")
          << Widget::click(std::bind(&MainMenuWindow::tool_clicked, this, _1, "texture")))
      << (Builder<menu_item>(px(0), px(40), px(100), px(20)) << Button::text("Players")
          << Widget::click(std::bind(&MainMenuWindow::tool_clicked, this, _1, "players")))
      << (Builder<menu_item>(px(0), px(60), px(100), px(20)) << Button::text("Pathing")
          << Widget::click(std::bind(&MainMenuWindow::tool_clicked, this, _1, "pathing")));

  fw::Framework *frmwrk = fw::Framework::get_instance();
  frmwrk->get_gui()->attach_widget(wnd_);
  frmwrk->get_gui()->attach_widget(file_menu_);
  frmwrk->get_gui()->attach_widget(tool_menu_);

  frmwrk->get_gui()->sig_click.connect(std::bind(&MainMenuWindow::global_click_handler, this, _1, _2, _3));
}

void MainMenuWindow::show() {
  wnd_->set_visible(true);
}

void MainMenuWindow::hide() {
  wnd_->set_visible(false);
}

/**
 * This is attached to the global GUI 'click' signal. If you've clicked on a widget that's not one
 * of our menus (or you clicked on blank space) then we need to hide the menus.
 */
void MainMenuWindow::global_click_handler(int button, bool is_down, fw::gui::Widget *w) {
  std::vector<Window *> menus = {file_menu_, tool_menu_};
  for(Window *menu : menus) {
    if (is_down && menu->is_visible() && !menu->is_child(w)) {
      menu->set_visible(false);
    }

    if (!is_down && menu->is_visible()) {
      menu->set_visible(false);
    }
  }
}

bool MainMenuWindow::file_menu_clicked(fw::gui::Widget *w) {
  file_menu_->set_visible(true);
  return true;
}

bool MainMenuWindow::tool_menu_clicked(fw::gui::Widget *w) {
  tool_menu_->set_visible(true);
  return true;
}

// when they click "File->New", we just show the "new map" window, which'll
// actually create the new map (assuming they click "OK" and that)
bool MainMenuWindow::file_new_clicked(fw::gui::Widget *w) {
  new_map->show();
  return true;
}

bool MainMenuWindow::file_save_clicked(fw::gui::Widget *w) {
  save_map->show();
  return true;
}

bool MainMenuWindow::file_open_clicked(fw::gui::Widget *w) {
  open_map->show();
  return true;
}

bool MainMenuWindow::file_quit_clicked(fw::gui::Widget *w) {
  // we don't actually "exit" the whole Application, just go back to the title Screen
  game::Application *app = dynamic_cast<game::Application *>(fw::Framework::get_instance()->get_app());
  game::ScreenStack *ss = app->get_screen_stack();
  ss->set_active_screen("title");
  return true;
}

void MainMenuWindow::map_screenshot_clicked_finished(std::shared_ptr<fw::Bitmap> bmp) {
/*  // re-show the UI, we don't need to hide it anymore!
  fw::Framework::get_instance()->get_gui()->get_root_window()->setVisible(true);

  // create a "thumbnail" version, 160x120
  fw::bitmap thumbnail(bmp);
  thumbnail.resize(160, 120, 3);

  editor_world *world = dynamic_cast<editor_world *>(ww::world::get_instance());
  world->set_screenshot(thumbnail);*/
}

// We want to take a screenshot of the map at the current camera view to display
// in the "New Game" and "Join Game" windows
bool MainMenuWindow::map_screenshot_clicked(fw::gui::Widget *w) {
  //fw::Framework::get_instance()->get_gui()->get_root_window()->setVisible(false);

  // we take the screenshot in 1024x768 because the maps expect a 4:3 image. We
  // then resize it ourselves to ensure it's nice & anti-aliased, etc.
  fw::Framework::get_instance()->take_screenshot(1024, 768,
      std::bind(&MainMenuWindow::map_screenshot_clicked_finished, this, std::placeholders::_1));

  return true;
}

// This is called when you click one of the "Tool" menu items. We figure out which
// one you clicked on and switch to that tool as appropriate.
bool MainMenuWindow::tool_clicked(fw::gui::Widget *w, std::string tool_name) {
  EditorScreen::get_instance()->set_active_tool(tool_name);
  return true;
}

//-------------------------------------------------------------------------
StatusbarWindow *statusbar = nullptr;

StatusbarWindow::StatusbarWindow() : wnd_(nullptr) {
}

StatusbarWindow::~StatusbarWindow() {
}

void StatusbarWindow::initialize() {
  wnd_ = Builder<Window>(px(0), sum(pct(100), px(-20)), pct(100), px(20)) << Window::background("frame")
      << Widget::visible(false)
      << (Builder<Label>(px(0), px(0), pct(100), pct(100)) << Widget::id(STATUS_MESSAGE_ID));
  fw::Framework *frmwrk = fw::Framework::get_instance();
  frmwrk->get_gui()->attach_widget(wnd_);
}

void StatusbarWindow::show() {
  wnd_->set_visible(true);
}

void StatusbarWindow::hide() {
  wnd_->set_visible(false);
}

void StatusbarWindow::set_message(std::string const &msg) {
  wnd_->find<Label>(STATUS_MESSAGE_ID)->set_text(msg);
}

}

#include <framework/bitmap.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/label.h>
#include <framework/gui/textedit.h>
#include <framework/gui/window.h>
#include <framework/gui/button.h>
#include <framework/misc.h>
#include <framework/texture.h>

#include <game/world/world.h>
#include <game/editor/editor_screen.h>
#include <game/editor/editor_world.h>
#include <game/editor/world_writer.h>
#include <game/editor/windows/save_map.h>

using namespace fw::gui;
using namespace std::placeholders;

namespace ed {

save_map_window *save_map = nullptr;

enum ids {
  NAME_ID,
  AUTHOR_ID,
  DESCRIPTION_ID,
  SCREENSHOT_ID
};

save_map_window::save_map_window() : _wnd(nullptr) {
}

save_map_window::~save_map_window() {
}

void save_map_window::initialize() {
  _wnd = builder<window>(sum(pct(50), px(-250)), sum(pct(50), px(-150)), px(500), px(232))
      << window::background("frame") << widget::visible(false)
      << (builder<label>(px(10), px(10), px(80), px(20)) << label::text("Name:"))
      << (builder<textedit>(px(90), px(10), px(160), px(20)) << widget::id(NAME_ID))
      << (builder<label>(px(10), px(40), px(80), px(20)) << label::text("Author:"))
      << (builder<textedit>(px(90), px(40), px(160), px(20)) << widget::id(AUTHOR_ID))
      << (builder<label>(px(10), px(70), px(80), px(20)) << label::text("Description:"))
      << (builder<textedit>(px(90), px(70), px(160), px(20)) << widget::id(DESCRIPTION_ID))
      << (builder<label>(px(260), px(10), px(230), px(172)) << widget::id(SCREENSHOT_ID))
      << (builder<button>(px(90), px(152), px(160), px(30)) << button::text("Update screenshot")
          << widget::click(std::bind(&save_map_window::screenshot_clicked, this, _1)))
      << (builder<button>(sum(pct(100), px(-180)), sum(pct(100), px(-38)), px(80), px(30)) << button::text("Save")
          << widget::click(std::bind(&save_map_window::save_clicked, this, _1)))
      << (builder<button>(sum(pct(100), px(-90)), sum(pct(100), px(-38)), px(80), px(30)) << button::text("Cancel")
          << widget::click(std::bind(&save_map_window::cancel_clicked, this, _1)));
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);
}

// when we go to show, we have to update our controls with what we currently know about the map we're editing.
void save_map_window::show() {
  _wnd->set_visible(true);

  auto world = dynamic_cast<editor_world *>(game::world::get_instance());

  _wnd->find<textedit>(NAME_ID)->set_text(world->get_name());
  _wnd->find<textedit>(DESCRIPTION_ID)->set_text(world->get_description());
  if (world->get_author() == "") {
    _wnd->find<textedit>(AUTHOR_ID)->set_text(fw::get_user_name());
  } else {
    _wnd->find<textedit>(AUTHOR_ID)->set_text(world->get_author());
  }
  update_screenshot();
}

// updates the screenshot that we're displaying whenever it changes.
void save_map_window::update_screenshot() {
  auto world = dynamic_cast<editor_world *>(game::world::get_instance());
  if (world->get_screenshot() == nullptr || world->get_screenshot()->get_width() == 0)
    return;

  std::shared_ptr<fw::bitmap> bmp = world->get_screenshot();
  _wnd->find<label>(SCREENSHOT_ID)->set_background(bmp);
}

bool save_map_window::screenshot_clicked(widget *w) {
  auto world = dynamic_cast<editor_world *>(game::world::get_instance());
  fw::framework::get_instance()->take_screenshot(
      1024, 768, std::bind(&save_map_window::screenshot_complete, this, _1), false);
  return true;
}

void save_map_window::screenshot_complete(std::shared_ptr<fw::bitmap> bitmap) {
  auto world = dynamic_cast<editor_world *>(game::world::get_instance());
  world->set_screenshot(bitmap);
  fw::framework::get_instance()->get_graphics()->run_on_render_thread([this]() {
    update_screenshot();
  });
}

bool save_map_window::save_clicked(widget *w) {
  auto world = dynamic_cast<editor_world *>(game::world::get_instance());
  world->set_name(_wnd->find<textedit>(NAME_ID)->get_text());
  world->set_author(_wnd->find<textedit>(AUTHOR_ID)->get_text());
  world->set_description(_wnd->find<textedit>(DESCRIPTION_ID)->get_text());

  world_writer writer(world);
  writer.write(world->get_name());

  _wnd->set_visible(false);
  return true;
}

bool save_map_window::cancel_clicked(widget *w) {
  _wnd->set_visible(false);
  return true;
}
}

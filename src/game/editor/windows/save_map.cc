#include <framework/framework.h>
#include <framework/texture.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/label.h>
#include <framework/gui/textedit.h>
#include <framework/gui/window.h>
#include <framework/gui/button.h>
#include <framework/bitmap.h>
#include <framework/misc.h>

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
  NAME,
  AUTHOR,
  DESCRIPTION,
};

save_map_window::save_map_window() : _wnd(nullptr) {
}

save_map_window::~save_map_window() {
}

void save_map_window::initialize() {
  _wnd = builder<window>(sum(pct(50), px(-150)), sum(pct(50), px(-75)), px(300), px(140))
      << window::background("frame") << widget::visible(false)
      << (builder<label>(px(10), px(10), px(80), px(20)) << label::text("Name:"))
      << (builder<textedit>(px(90), px(10), px(140), px(20)) << widget::id(NAME))
      << (builder<label>(px(10), px(40), px(80), px(20)) << label::text("Author:"))
      << (builder<textedit>(px(90), px(40), px(140), px(20)) << widget::id(AUTHOR))
      << (builder<label>(px(10), px(70), px(80), px(20)) << label::text("Description:"))
      << (builder<textedit>(px(90), px(70), px(140), px(20)) << widget::id(DESCRIPTION))
      << (builder<button>(sum(pct(100), px(-180)), sum(pct(100), px(-28)), px(80), px(20)) << button::text("Save")
          << widget::click(std::bind(&save_map_window::save_clicked, this, _1)))
      << (builder<button>(sum(pct(100), px(-90)), sum(pct(100), px(-28)), px(80), px(20)) << button::text("Cancel")
          << widget::click(std::bind(&save_map_window::cancel_clicked, this, _1)));
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);
}

// when we go to show, we have to update our controls with what we currently know about
// the map we're editing.
void save_map_window::show() {
  _wnd->set_visible(true);

  auto world = dynamic_cast<editor_world *>(rp::world::get_instance());

  _wnd->find<textedit>(NAME)->set_text(world->get_name());
  _wnd->find<textedit>(DESCRIPTION)->set_text(world->get_description());
  if (world->get_author() == "") {
    _wnd->find<textedit>(AUTHOR)->set_text(fw::get_user_name());
  } else {
    _wnd->find<textedit>(AUTHOR)->set_text(world->get_author());
  }
  update_screenshot();
}

// updates the screenshot that we're displaying whenever it changes.
void save_map_window::update_screenshot() {
  auto world = dynamic_cast<editor_world *>(rp::world::get_instance());
  if (world->get_screenshot() == nullptr || world->get_screenshot()->get_width() == 0)
    return;

  std::shared_ptr<fw::bitmap> bmp = world->get_screenshot();
}

bool save_map_window::save_clicked(widget *w) {
  auto world = dynamic_cast<editor_world *>(rp::world::get_instance());
  world->set_name(_wnd->find<textedit>(NAME)->get_text());
  world->set_author(_wnd->find<textedit>(AUTHOR)->get_text());
  world->set_description(_wnd->find<textedit>(DESCRIPTION)->get_text());

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

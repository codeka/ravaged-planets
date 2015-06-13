
#include <functional>
#include <boost/lexical_cast.hpp>

#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/gui.h>
#include <framework/gui/window.h>
#include <framework/gui/button.h>
#include <framework/gui/label.h>
#include <framework/gui/textedit.h>
#include <game/application.h>
#include <game/screens/screen.h>
#include <game/world/terrain.h>

#include <game/editor/windows/new_map.h>
#include <game/editor/windows/message_box.h>
#include <game/editor/editor_screen.h>

namespace ed {

using namespace fw::gui;
using namespace std::placeholders;

new_map_window *new_map = nullptr;

static const int WIDTH_ID = 1;
static const int HEIGHT_ID = 2;

new_map_window::new_map_window() : _wnd(nullptr) {
}

new_map_window::~new_map_window() {
}

void new_map_window::initialize() {
  _wnd = builder<window>(sum(pct(50), px(-100)), sum(pct(50), px(-100)), px(200), px(100))
          << window::background("frame") << widget::visible(false)
      << (builder<label>(px(10), px(10), sum(pct(100), px(-20)), px(18)) << label::text("Size:"))
      << (builder<textedit>(px(10), px(30), sum(pct(50), px(-20)), px(20))
          << textedit::text("4") << widget::id(WIDTH_ID))
      << (builder<label>(sum(pct(50), px(-8)), px(30), px(16), px(20)) << label::text("x"))
      << (builder<textedit>(sum(pct(50), px(10)), px(30), sum(pct(50), px(-20)), px(20))
          << textedit::text("4") << widget::id(HEIGHT_ID))
      << (builder<button>(sum(pct(100), px(-180)), sum(pct(100), px(-28)), px(80), px(20)) << button::text("Create")
          << widget::click(std::bind(&new_map_window::ok_clicked, this, _1)))
      << (builder<button>(sum(pct(100), px(-90)), sum(pct(100), px(-28)), px(80), px(20)) << button::text("Cancel")
          << widget::click(std::bind(&new_map_window::cancel_clicked, this, _1)));
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);
}

void new_map_window::show() {
  _wnd->set_visible(true);
}

void new_map_window::hide() {
  _wnd->set_visible(false);
}

bool new_map_window::ok_clicked(widget *w) {
  _wnd->set_visible(false);

  int width;
  int height;
  try {
    width = boost::lexical_cast<int>(_wnd->find<textedit>(WIDTH_ID)->get_text());
    height = boost::lexical_cast<int>(_wnd->find<textedit>(HEIGHT_ID)->get_text());
  } catch (boost::bad_lexical_cast &) {
//    message_box->show("Invalid Parameters", "Width and Height must be an integer.");
    return true;
  }

  editor_screen::get_instance()->new_map(width * rp::terrain::PATCH_SIZE, width * rp::terrain::PATCH_SIZE);
  return true;
}

bool new_map_window::cancel_clicked(widget *w) {
  _wnd->set_visible(false);
  return true;
}

}

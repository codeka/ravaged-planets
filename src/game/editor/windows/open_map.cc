
#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/gui.h>
#include <framework/gui/window.h>
#include <framework/gui/button.h>
#include <framework/gui/label.h>
#include <framework/gui/listbox.h>

#include <game/world/world.h>
#include <game/world/world_reader.h>
#include <game/world/world_vfs.h>
#include <game/editor/editor_screen.h>
#include <game/editor/windows/open_map.h>

using namespace fw::gui;
using namespace std::placeholders;

namespace ed {

open_map_window *open_map = nullptr;

enum ids {
  MAP_LIST
};

open_map_window::open_map_window() : _wnd(nullptr) {
}

open_map_window::~open_map_window() {
}

void open_map_window::initialize() {
  _wnd = builder<window>(sum(pct(50), px(-100)), sum(pct(50), px(-150)), px(200), px(200))
      << window::background("frame") << widget::visible(false)
      << (builder<listbox>(px(10), px(10), sum(pct(100), px(-20)), sum(pct(100), px(-50))) << widget::id(MAP_LIST))
      << (builder<button>(sum(pct(100), px(-180)), sum(pct(100), px(-28)), px(80), px(20)) << button::text("Open")
          << widget::click(std::bind(&open_map_window::open_clicked, this, _1)))
      << (builder<button>(sum(pct(100), px(-90)), sum(pct(100), px(-28)), px(80), px(20)) << button::text("Cancel")
          << widget::click(std::bind(&open_map_window::cancel_clicked, this, _1)));
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);

  for (int i = 0; i < 20; i++) {
    std::string title = "Item # " + std::to_string(i);
    _wnd->find<listbox>(MAP_LIST)->add_item(builder<label>(px(0), px(0), pct(100), px(20)) << label::text(title));
  }

/*
  _maps = get_child < CEGUI::ItemListbox > ("OpenMap/MapList");
  CEGUI::WindowManager &wndmgr = CEGUI::WindowManager::getSingleton();

  ww::world_vfs vfs;
  std::vector<ww::world_summary> map_list = vfs.list_maps();

  _maps->resetList();
  BOOST_FOREACH(ww::world_summary & ws, map_list)
  {
    CEGUI::ItemEntry *entry = dynamic_cast<CEGUI::ItemEntry *>(wndmgr.createWindow("ww/ListboxItem",
        ("OpenMap/MapList/" + ws.get_name()).c_str()));
    entry->setText(ws.get_name().c_str());
    _maps->addItem(entry);
  }*/
}

bool open_map_window::open_clicked(widget *w) {
  //editor_screen::get_instance()->open_map(map_name);

  hide();
  return true;
}

bool open_map_window::cancel_clicked(widget *w) {
  hide();
  return true;
}

void open_map_window::show() {
  _wnd->set_visible(true);
}

void open_map_window::hide() {
  _wnd->set_visible(false);
}

}

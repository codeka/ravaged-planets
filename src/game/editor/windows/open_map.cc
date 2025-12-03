#include <vector>

#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/gui.h>
#include <framework/gui/widget.h>
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

OpenMapWindow *open_map = nullptr;

enum ids {
  MAP_LIST
};

OpenMapWindow::OpenMapWindow() : wnd_(nullptr) {
}

OpenMapWindow::~OpenMapWindow() {
}

void OpenMapWindow::initialize() {
  wnd_ = Builder<Window>(sum(pct(50), px(-100)), sum(pct(50), px(-150)), px(200), px(200))
      << Window::background("frame") << Widget::visible(false)
      << (Builder<Listbox>(px(10), px(10), sum(pct(100), px(-20)), sum(pct(100), px(-50))) << Widget::id(MAP_LIST))
      << (Builder<Button>(sum(pct(100), px(-180)), sum(pct(100), px(-28)), px(80), px(20)) << Button::text("Open")
          << Widget::click(std::bind(&OpenMapWindow::open_clicked, this, _1)))
      << (Builder<Button>(sum(pct(100), px(-90)), sum(pct(100), px(-28)), px(80), px(20)) << Button::text("Cancel")
          << Widget::click(std::bind(&OpenMapWindow::cancel_clicked, this, _1)));
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);

  game::WorldVfs vfs;
  std::vector<game::WorldSummary> map_list = vfs.list_maps();
  for(game::WorldSummary &ws : map_list) {
    std::string title = ws.get_name();
    wnd_->find<Listbox>(MAP_LIST)->add_item(
        Builder<Label>(px(0), px(0), pct(100), px(20)) << Label::text(title) << Widget::data(ws));
  }
}

bool OpenMapWindow::open_clicked(Widget *w) {
  Widget *selected_widget = wnd_->find<Listbox>(MAP_LIST)->get_selected_item();
  if (selected_widget == nullptr) {
    return true;
  }

  game::WorldSummary const &ws = std::any_cast<game::WorldSummary const &>(selected_widget->get_data());
  auto status = EditorScreen::get_instance()->open_map(ws.get_name());
  if (!status.ok()) {
    // TODO: show error to user?
    fw::debug << "Error opening map: " << status << std::endl;
    return true;
  }

  hide();
  return true;
}

bool OpenMapWindow::cancel_clicked(Widget *w) {
  hide();
  return true;
}

void OpenMapWindow::show() {
  wnd_->set_visible(true);
}

void OpenMapWindow::hide() {
  wnd_->set_visible(false);
}

}

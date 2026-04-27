#include <vector>

#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/gui.h>
#include <framework/gui/widget.h>
#include <framework/gui/window.h>
#include <framework/gui/button.h>
#include <framework/gui/label.h>
#include <framework/gui/listbox.h>
#include <framework/service_locator.h>

#include <game/world/world.h>
#include <game/world/world_reader.h>
#include <game/world/world_vfs.h>
#include <game/editor/editor_screen.h>
#include <game/editor/windows/open_map.h>

using namespace fw::gui;
using namespace std::placeholders;

namespace ed {

std::unique_ptr<OpenMapWindow> open_map;

enum ids {
  MAP_LIST = 8735
};

OpenMapWindow::OpenMapWindow() : wnd_(nullptr) {
}

OpenMapWindow::~OpenMapWindow() {
}

void OpenMapWindow::initialize() {
  wnd_ = Builder<Window>()
		  << Widget::width(Widget::Fixed(400.f))
      << Widget::height(Widget::Fixed(200.f))
		  << Window::initial_position(WindowInitialPosition::Center())
      << Widget::background("frame")
      << Widget::visible(false)
      << (Builder<Listbox>()
          << Widget::width(Widget::MatchParent())
          << Widget::height(Widget::MatchParent())
				  << Widget::margin(10.f, 10.f, 50.f, 10.f)
          << Widget::id(MAP_LIST))
      << (Builder<Button>()
          << Button::text("Open")
          << Widget::width(Widget::Fixed(100.f))
          << Widget::height(Widget::Fixed(30.f))
          << Widget::margin(0.f, 10.f, 10.f, 0.f)
				  << Widget::gravity(LayoutParams::Gravity::kBottom | LayoutParams::Gravity::kRight)
          << Widget::click(std::bind(&OpenMapWindow::open_clicked, this, _1)))
      << (Builder<Button>()
          << Button::text("Cancel")
          << Widget::width(Widget::Fixed(100.f))
          << Widget::height(Widget::Fixed(30.f))
          << Widget::margin(0.f, 120.f, 10.f, 0.f)
          << Widget::gravity(LayoutParams::Gravity::kBottom | LayoutParams::Gravity::kRight)
          << Widget::click(std::bind(&OpenMapWindow::cancel_clicked, this, _1)));

  fw::Get<Gui>().AttachWindow(wnd_);
}

bool OpenMapWindow::open_clicked(Widget &w) {
  auto selected_widget = wnd_->Find<Listbox>(MAP_LIST)->GetSelectedItem();
  if (!selected_widget) {
    return true;
  }

  game::WorldSummary const &ws =
      std::any_cast<game::WorldSummary const &>(selected_widget->get_data());
  auto status = EditorScreen::get_instance()->open_map(ws.get_name());
  if (!status.ok()) {
    // TODO: show error to user?
    LOG(ERR) << "error opening map: " << status << std::endl;
    return true;
  }

  hide();
  return true;
}

bool OpenMapWindow::cancel_clicked(Widget &w) {
  hide();
  return true;
}

void OpenMapWindow::show() {
  auto map_list = wnd_->Find<Listbox>(MAP_LIST);

  game::WorldVfs vfs;
  auto maps = vfs.list_maps();

  fw::Get<fw::Graphics>().run_on_render_thread([maps, map_list]() {
    for (game::WorldSummary const &ws : maps) {
      std::string title = ws.get_name();
      map_list->AddItem(
        Builder<Label>()
        << Widget::width(Widget::MatchParent())
        << Widget::height(Widget::WrapContent())
        << Widget::padding(4.f, 4.f, 4.f, 4.f)
        << Label::text(title)
        << Widget::data(ws));
    }
  });

  wnd_->set_visible(true);
}

void OpenMapWindow::hide() {
  wnd_->set_visible(false);
}

}

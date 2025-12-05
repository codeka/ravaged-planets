
#include <functional>

#include <absl/strings/numbers.h>

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

NewMapWindow *new_map = nullptr;

static const int WIDTH_ID = 1;
static const int HEIGHT_ID = 2;

NewMapWindow::NewMapWindow() : wnd_(nullptr) {
}

NewMapWindow::~NewMapWindow() {
}

void NewMapWindow::initialize() {
  wnd_ = Builder<Window>(sum(pct(50), px(-100)), sum(pct(50), px(-100)), px(200), px(100))
          << Window::background("frame") << Widget::visible(false)
      << (Builder<Label>(px(10), px(10), sum(pct(100), px(-20)), px(18)) << Label::text("Size:"))
      << (Builder<TextEdit>(px(10), px(30), sum(pct(50), px(-20)), px(20))
          << TextEdit::text("4") << Widget::id(WIDTH_ID))
      << (Builder<Label>(sum(pct(50), px(-8)), px(30), px(16), px(20)) << Label::text("x"))
      << (Builder<TextEdit>(sum(pct(50), px(10)), px(30), sum(pct(50), px(-20)), px(20))
          << TextEdit::text("4") << Widget::id(HEIGHT_ID))
      << (Builder<Button>(sum(pct(100), px(-180)), sum(pct(100), px(-28)), px(80), px(20)) << Button::text("Create")
          << Widget::click(std::bind(&NewMapWindow::ok_clicked, this, _1)))
      << (Builder<Button>(sum(pct(100), px(-90)), sum(pct(100), px(-28)), px(80), px(20)) << Button::text("Cancel")
          << Widget::click(std::bind(&NewMapWindow::cancel_clicked, this, _1)));
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);
}

void NewMapWindow::show() {
  wnd_->set_visible(true);
}

void NewMapWindow::hide() {
  wnd_->set_visible(false);
}

bool NewMapWindow::ok_clicked(Widget *w) {
  wnd_->set_visible(false);

  int width;
  int height;

  if (!absl::SimpleAtoi(wnd_->find<TextEdit>(WIDTH_ID)->get_text(), &width) ||
      !absl::SimpleAtoi(wnd_->find<TextEdit>(HEIGHT_ID)->get_text(), &height)) {
    // TODO: show error
//    message_box->show("Invalid Parameters", "Width and Height must be an integer.");
    return true;
  }

  EditorScreen::get_instance()->new_map(width * game::Terrain::PATCH_SIZE, width * game::Terrain::PATCH_SIZE);
  return true;
}

bool NewMapWindow::cancel_clicked(Widget *w) {
  wnd_->set_visible(false);
  return true;
}

}


#include <functional>
#include <memory>

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
#include <framework/gui/linear_layout.h>

namespace ed {

using namespace fw::gui;
using namespace std::placeholders;

std::unique_ptr<NewMapWindow> new_map;

static const int WIDTH_ID = 1;
static const int HEIGHT_ID = 2;

NewMapWindow::NewMapWindow() : wnd_(nullptr) {
}

NewMapWindow::~NewMapWindow() {
}

void NewMapWindow::initialize() {
  wnd_ = Builder<Window>()
      << Widget::width(Widget::Fixed(400.f))
      << Widget::height(Widget::WrapContent())
      << Widget::background("frame")
      << Window::initial_position(WindowInitialPosition::Center())
      << Widget::visible(false)
      << (Builder<LinearLayout>()
          << Widget::width(Widget::MatchParent())
          << Widget::height(Widget::WrapContent())
          << LinearLayout::orientation(LinearLayout::Orientation::kVertical)
          << (Builder<Label>()
              << Widget::width(Widget::MatchParent())
              << Widget::height(Widget::WrapContent())
              << Widget::margin(10.f, 10.f, 5.f, 10.f)
              << Label::text("Size:"))
          << (Builder<LinearLayout>()
            << Widget::width(Widget::MatchParent())
            << Widget::height(Widget::WrapContent())
            << Widget::margin(5.f, 10.f, 5.f, 10.f)
            << LinearLayout::orientation(LinearLayout::Orientation::kHorizontal)
                << (Builder<TextEdit>()
                    << LinearLayout::weight(1.0f)
                    << Widget::height(Widget::WrapContent())
                    << Widget::padding(4.f, 4.f, 4.f, 4.f)
                    << TextEdit::text("4")
                    << Widget::id(WIDTH_ID))
                << (Builder<Label>()
                    << Widget::width(Widget::WrapContent())
                    << Widget::height(Widget::WrapContent())
                    << Widget::margin(0.f, 10.f, 0.f, 10.f)
                    << Label::text("x"))
                << (Builder<TextEdit>()
                  << LinearLayout::weight(1.0f)
                  << Widget::height(Widget::WrapContent())
                  << Widget::padding(4.f, 4.f, 4.f, 4.f)
                  << TextEdit::text("4")
                  << Widget::id(HEIGHT_ID))
          )
          << (Builder<LinearLayout>()
            << Widget::width(Widget::MatchParent())
            << Widget::height(Widget::WrapContent())
            << Widget::margin(5.f, 10.f, 10.f, 10.f)
            << LinearLayout::orientation(LinearLayout::Orientation::kHorizontal)
            << (Builder<Widget>()
                << LinearLayout::weight(1.0f)
                << Widget::height(Widget::Fixed(1.f)))
            << (Builder<Button>()
                << Widget::width(Widget::Fixed(100.f))
                << Widget::height(Widget::Fixed(30.f))
                << Widget::margin(0.f, 5.f, 0.f, 0.f)
                << Button::text("Cancel")
                << Widget::click(std::bind(&NewMapWindow::cancel_clicked, this, _1)))
            << (Builder<Button>()
              << Widget::width(Widget::Fixed(100.f))
              << Widget::height(Widget::Fixed(30.f))
              << Widget::margin(0.f, 0.f, 0.f, 5.f)
              << Button::text("Create")
              << Widget::click(std::bind(&NewMapWindow::ok_clicked, this, _1)))
      ));
  fw::Get<Gui>().AttachWindow(wnd_);
}

void NewMapWindow::show() {
  wnd_->set_visible(true);
}

void NewMapWindow::hide() {
  wnd_->set_visible(false);
}

bool NewMapWindow::ok_clicked(Widget &w) {
  wnd_->set_visible(false);

  int width;
  int height;

  if (!absl::SimpleAtoi(wnd_->Find<TextEdit>(WIDTH_ID)->get_text(), &width) ||
      !absl::SimpleAtoi(wnd_->Find<TextEdit>(HEIGHT_ID)->get_text(), &height)) {
    message_box->show("Invalid Parameters", "Width and Height must be an integer.", []() {});
    return true;
  }

  EditorScreen::get_instance()->new_map(width * game::Terrain::PATCH_SIZE, width * game::Terrain::PATCH_SIZE);
  return true;
}

bool NewMapWindow::cancel_clicked(Widget &w) {
  wnd_->set_visible(false);
  return true;
}

}

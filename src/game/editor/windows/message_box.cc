#include "message_box.h"

#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/label.h>
#include <framework/gui/widget.h>
#include <framework/gui/linear_layout.h>

namespace ed {
namespace {

using namespace fw::gui;
using namespace std::placeholders;

constexpr int MESSAGE_BOX_CAPTION_ID = 1;
constexpr int MESSAGE_BOX_MESSAGE_ID = 2;

}

std::unique_ptr<MessageBoxWindow> message_box;

void MessageBoxWindow::initialize() {
  wnd_ = Builder<Window>()
      << Widget::width(LayoutParams::Mode::kFixed, 240.f)
      << Widget::height(LayoutParams::Mode::kWrapContent, 0)
		  << Window::initial_position(WindowInitialPosition::Center())
      << Window::background("frame") << Widget::visible(false)
      << (Builder<LinearLayout>()
          << Widget::width(LayoutParams::Mode::kMatchParent, 0)
          << Widget::height(LayoutParams::Mode::kWrapContent, 0)
				  << LinearLayout::orientation(LinearLayout::Orientation::kVertical)
          << (Builder<Label>()
              << Widget::width(LayoutParams::Mode::kMatchParent, 0)
              << Widget::height(LayoutParams::Mode::kWrapContent, 0)
              << Widget::id(MESSAGE_BOX_CAPTION_ID))
          << (Builder<Label>()
              << Widget::width(LayoutParams::Mode::kMatchParent, 0)
              << Widget::height(LayoutParams::Mode::kWrapContent, 0)
              << Widget::id(MESSAGE_BOX_MESSAGE_ID))
          << (Builder<Button>()
              << Widget::width(LayoutParams::Mode::kFixed, 100.f)
              << Widget::height(LayoutParams::Mode::kFixed, 20.f)
							<< Widget::margin(10.f, 0.f, 10.f, 0.f)
						  << Widget::gravity(LayoutParams::Gravity::kCenterHorizontal)
              << Button::text("OK")
              << Widget::click(std::bind(&MessageBoxWindow::ok_clicked, this, _1))));
  fw::Get<Gui>().AttachWindow(wnd_);
}

void MessageBoxWindow::show(
    std::string_view caption,
    std::string_view message,
    std::function<void()> ok_click_handler) {
  ok_click_handler_ = ok_click_handler;
  wnd_->set_visible(true);

  auto caption_label = wnd_->Find<Label>(MESSAGE_BOX_CAPTION_ID);
  caption_label->set_text(caption);

  auto message_label = wnd_->Find<Label>(MESSAGE_BOX_MESSAGE_ID);
  message_label->set_text(message);
}

bool MessageBoxWindow::ok_clicked(fw::gui::Widget& w) {
  if (ok_click_handler_) {
    ok_click_handler_();
	}
	ok_click_handler_ = nullptr;
  wnd_->set_visible(false);
  return true;
}

}

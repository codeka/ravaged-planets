#include "message_box.h"

#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/label.h>
#include <framework/gui/widget.h>

namespace ed {
namespace {

using namespace fw::gui;
using namespace std::placeholders;

constexpr int MESSAGE_BOX_CAPTION_ID = 1;
constexpr int MESSAGE_BOX_MESSAGE_ID = 2;

}

std::unique_ptr<MessageBoxWindow> message_box;

void MessageBoxWindow::initialize() {
  wnd_ = Builder<Window>(sum(pct(50), px(-100)), sum(pct(50), px(-100)), px(200), px(100))
      << Window::background("frame") << Widget::visible(false)
      << (Builder<Label>(px(10), px(10), sum(pct(100), px(-20)), px(18))
          << Widget::id(MESSAGE_BOX_CAPTION_ID))
    << (Builder<Label>(px(10), px(30), sum(pct(100), px(-20)), px(18))
        << Widget::id(MESSAGE_BOX_MESSAGE_ID))
    << (Builder<Button>(sum(pct(100), px(-180)), sum(pct(100), px(-28)), px(80), px(20))
        << Button::text("OK")
        << Widget::click(std::bind(&MessageBoxWindow::ok_clicked, this, _1)));
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);
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

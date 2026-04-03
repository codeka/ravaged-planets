#pragma once

#include <functional>
#include <memory>
#include <string_view>

#include <framework/gui/widget.h>
#include <framework/gui/window.h>

namespace ed {

// This is a simple message box that lets you display a simple message
// to the user with an "OK" button.
class MessageBoxWindow {
private:
  std::shared_ptr<fw::gui::Window> wnd_;
  std::function<void()> ok_click_handler_;

  bool ok_clicked(fw::gui::Widget& w);

public:
  MessageBoxWindow() = default;
  virtual ~MessageBoxWindow() = default;

  void initialize();
  void show(
	  std::string_view caption,
	  std::string_view message,
	  std::function<void()> ok_click_handler);
};

extern std::unique_ptr<MessageBoxWindow> message_box;

}

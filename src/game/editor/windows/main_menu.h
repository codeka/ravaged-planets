#pragma once

#include <framework/signals.h>

namespace fw {
namespace gui {
class Window;
class Widget;
}
class bitmap;
}

namespace ed {
class EditorScreen;

// Represents the main menu ("File", "Edit", and all that jazz)
class MainMenuWindow {
private:
  std::shared_ptr<fw::gui::Window> wnd_;
  std::shared_ptr<fw::gui::Window> file_menu_;
  std::shared_ptr<fw::gui::Window> tool_menu_;
  fw::SignalConnection global_click_conn_;

  void global_click_handler(int button, bool is_down, fw::gui::Widget const &w);
  void global_click_away_handler(int button, bool is_down);
  bool file_menu_clicked(fw::gui::Widget &w);
  bool tool_menu_clicked(fw::gui::Widget &w);
  bool file_new_clicked(fw::gui::Widget &w);
  bool file_save_clicked(fw::gui::Widget &w);
  bool file_open_clicked(fw::gui::Widget &w);
  bool file_quit_clicked(fw::gui::Widget &w);
  bool tool_clicked(fw::gui::Widget &w, std::string tool_name);

  void map_screenshot_clicked_finished(fw::Bitmap const &bmp);
  bool map_screenshot_clicked(fw::gui::Widget *w);

public:
  MainMenuWindow();
  ~MainMenuWindow();

  void initialize();
  void show();
  void hide();
};

// represents the statusbar
class StatusbarWindow {
private:
  std::shared_ptr<fw::gui::Window> wnd_;
public:
  StatusbarWindow();
  ~StatusbarWindow();

  void initialize();
  void show();
  void hide();

  void set_message(std::string const &msg);
};

extern MainMenuWindow *main_menu;
extern StatusbarWindow *statusbar;

}

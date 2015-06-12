#pragma once

namespace fw {
namespace gui {
class window;
class widget;
}
class bitmap;
}

namespace ed {
class editor_screen;

// Represents the main menu ("File", "Edit", and all that jazz)
class main_menu_window {
private:
  fw::gui::window *_wnd;
  fw::gui::window *_file_menu;

  void global_click_handler(int button, bool is_down, fw::gui::widget *w);
  bool file_clicked(fw::gui::widget *w);
  bool file_new_clicked(fw::gui::widget *w);
  bool file_save_clicked(fw::gui::widget *w);
  bool file_open_clicked(fw::gui::widget *w);
  bool file_quit_clicked(fw::gui::widget *w);
  bool tool_clicked(fw::gui::widget *w);

  void map_screenshot_clicked_finished(fw::bitmap const &bmp);
  bool map_screenshot_clicked(fw::gui::widget *w);

public:
  main_menu_window();
  virtual ~main_menu_window();

  virtual void initialize();
};
/*
 // represents the statusbar
 class statusbar_window
 {
 private:
 public:
 statusbar_window();
 virtual ~statusbar_window();

 virtual void initialize();

 void set_message(std::string const &msg);
 };
 */
extern main_menu_window *main_menu;
//	extern statusbar_window *statusbar;

}

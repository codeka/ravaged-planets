#pragma once

#include <functional>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace fw {
namespace gui {
class Listbox;
class Widget;
class Window;
}
}

namespace ed {

// A generic "open file" dialog, which lets us open whatever file we want to open, though with a
// particular focus on opening images.
class OpenFileWindow {
public:
  typedef std::function<void(OpenFileWindow *)> file_selected_handler;

private:
  fw::gui::Window *wnd_;
  boost::filesystem::path _curr_directory;
  file_selected_handler _file_selected_handler;
  std::vector<std::string> _items;
  bool _show_hidden;

  bool on_ok_clicked(fw::gui::Widget *w);
  bool on_cancel_clicked(fw::gui::Widget *w);
  void on_item_selected(int index);
  void on_item_activated(int index);
  bool on_show_hidden_clicked(fw::gui::Widget *w);

  void refresh();
  void navigate_to_directory(boost::filesystem::path const &new_directory);
  void add_row(fw::gui::Listbox *lbx, std::string const &name);

public:
  OpenFileWindow();
  ~OpenFileWindow();

  void initialize();
  void show(file_selected_handler fn);
  void hide();

  boost::filesystem::path get_selected_file() const;
};

extern OpenFileWindow *open_file;
}

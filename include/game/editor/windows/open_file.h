#pragma once

#include <functional>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace fw {
namespace gui {
class listbox;
class widget;
class window;
}
}

namespace ed {
/**
 * A generic "open file" dialog, which lets us open whatever file we want to open, though with a
 * particular focus on opening images.
 */
class open_file_window {
public:
  typedef std::function<void(open_file_window *)> file_selected_handler;

private:
  fw::gui::window *_wnd;
  boost::filesystem::path _curr_directory;
  file_selected_handler _file_selected_handler;
  std::vector<std::string> _items;

  bool on_ok_clicked(fw::gui::widget *w);
  bool on_cancel_clicked(fw::gui::widget *w);
  void on_item_selected(int index);
  void on_item_activated(int index);

  void refresh_filelist();
  void navigate_to_directory(boost::filesystem::path const &new_directory);
  void add_row(fw::gui::listbox *lbx, std::string const &name);

public:
  open_file_window();
  ~open_file_window();

  void initialize();
  void show(file_selected_handler fn);
  void hide();

  boost::filesystem::path get_selected_file() const;
};

extern open_file_window *open_file;
}

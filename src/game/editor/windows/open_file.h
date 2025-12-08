#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

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
  typedef std::function<void(OpenFileWindow &)> FileSelectedHandler;

  OpenFileWindow();
  ~OpenFileWindow();

  void Initialize();
  void Show(FileSelectedHandler fn);
  void Hide();

  std::filesystem::path get_selected_file() const;

  private:
  std::shared_ptr<fw::gui::Window> wnd_;
  std::filesystem::path curr_directory_;
  FileSelectedHandler file_selected_handler_;
  std::vector<std::string> items_;
  bool show_hidden_;

  bool OnOkClicked(fw::gui::Widget &w);
  bool OnCancelClicked(fw::gui::Widget &w);
  void OnItemSelected(int index);
  void OnItemActivated(int index);
  bool OnShowHiddenClicked(fw::gui::Widget &w);

  void Refresh();
  void NavigateToDirectory(std::filesystem::path const &new_directory);
  void AddRow(fw::gui::Listbox &lbx, std::string_view name);

};

extern OpenFileWindow *open_file;
}

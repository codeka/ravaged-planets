#include <filesystem>
#include <functional>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <framework/exception.h>
#include <framework/bitmap.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/checkbox.h>
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>
#include <framework/gui/listbox.h>
#include <framework/gui/textedit.h>
#include <framework/gui/widget.h>
#include <framework/gui/window.h>
#include <framework/logging.h>
#include <framework/paths.h>
#include <framework/texture.h>

#include <game/editor/windows/open_file.h>

namespace fs = std::filesystem;
using namespace fw::gui;
using namespace std::placeholders;

static std::string format_file_size(uint32_t size);

namespace ed {

enum IDS {
  FILE_LIST_ID,
  FILENAME_ID,
  OK_ID,
  IMAGE_PREVIEW_ID
};

OpenFileWindow *open_file = nullptr;

OpenFileWindow::OpenFileWindow() : wnd_(nullptr), _show_hidden(false) {
}

OpenFileWindow::~OpenFileWindow() {
  fw::Framework::get_instance()->get_gui()->detach_widget(wnd_);
}

void OpenFileWindow::initialize() {
  wnd_ = Builder<Window>(sum(pct(50), px(-200)), sum(pct(40), px(-150)), px(400), px(300))
      << Window::background("frame") << Widget::visible(false)
      << (Builder<TextEdit>(px(8), px(8), sum(pct(100), px(-16)), px(18)) << Widget::id(FILENAME_ID))
      << (Builder<Listbox>(px(8), px(30), sum(pct(66), px(-12)), sum(pct(100), px(-76))) << Widget::id(FILE_LIST_ID)
          << Listbox::item_selected(std::bind(&OpenFileWindow::on_item_selected, this, _1))
          << Listbox::item_activated(std::bind(&OpenFileWindow::on_item_activated, this, _1)))
      << (Builder<Label>(sum(pct(66), px(4)), px(30), sum(pct(33), px(-12)), px(100)) << Widget::id(IMAGE_PREVIEW_ID))
      << (Builder<Button>(sum(pct(100), px(-176)), sum(pct(100), px(-38)), px(80), px(30))
          << Button::text("OK") << Widget::id(OK_ID)
          << Button::click(std::bind(&OpenFileWindow::on_ok_clicked, this, _1)))
      << (Builder<Button>(sum(pct(100), px(-88)), sum(pct(100), px(-38)), px(80), px(30))
          << Button::text("Cancel") << Button::click(std::bind(&OpenFileWindow::on_cancel_clicked, this, _1)))
      << (Builder<Checkbox>(px(8), sum(pct(100), px(-32)), px(150), px(18)) << Checkbox::text("Show hidden files")
          << Widget::click(std::bind(&OpenFileWindow::on_show_hidden_clicked, this, _1)));
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);
  _curr_directory = fw::user_base_path();
}

void OpenFileWindow::show(file_selected_handler fn) {
  _file_selected_handler = fn;
  wnd_->set_visible(true);
  refresh();
}

void OpenFileWindow::hide() {
  wnd_->set_visible(false);
}

void OpenFileWindow::add_row(Listbox *lbx, std::string const &name) {
  fs::path full_path = _curr_directory / name;

  std::string file_size = "";
  if (fs::is_regular_file(full_path)) {
    file_size = format_file_size(fs::file_size(full_path));
  }

  std::string icon_name = fs::is_directory(full_path) ? "editor_icon_directory" : "editor_icon_file";

  lbx->add_item(Builder<Widget>(px(0), px(0), pct(100), px(20))
      << (Builder<Label>(px(0), px(0), px(20), px(20))
          << Label::background(icon_name, true /* centered */))
      << (Builder<Label>(px(28), px(0), sum(pct(75), px(-28)), px(20)) << Label::text(name))
      << (Builder<Label>(pct(75), px(0), pct(25), px(20))
          << Label::text(file_size) << Label::text_align(Label::kRight)));
  _items.push_back(name);
}

void OpenFileWindow::refresh() {
  Listbox*lbx = wnd_->find<Listbox>(FILE_LIST_ID);
  lbx->clear();
  _items.clear();

  _curr_directory = fs::canonical(_curr_directory);
  if (_curr_directory != _curr_directory.root_name()) {
    // if it's not the root folder, add a ".." entry
    add_row(lbx, "..");
  }

  // Add them all to a directory so that we can sort them.
  std::vector<fs::path> paths;
  for (fs::directory_iterator it(_curr_directory); it != fs::directory_iterator(); ++it) {
    if (!_show_hidden && fw::is_hidden(it->path())) {
      continue;
    }
    paths.push_back(it->path());
  }

  // Sort them by name, but with directories first
  std::sort(paths.begin(), paths.end(), [](fs::path const &lhs, fs::path const &rhs) {
    bool lhs_is_dir = fs::is_directory(lhs);
    bool rhs_is_dir = fs::is_directory(rhs);
    if (lhs_is_dir && !rhs_is_dir) {
      return true;
    } else if (rhs_is_dir && !lhs_is_dir) {
      return false;
    } else {
      std::string lhs_file = lhs.filename().string();
      std::string rhs_file = rhs.filename().string();
      return boost::ilexicographical_compare(lhs_file, rhs_file);
    }
  });

  // Now add them to the listbox
  for(fs::path path : paths) {
    add_row(lbx, path.filename().string());
  }

  // we also want to set the "Path" to be the full path that we're currently displaying
  wnd_->find<TextEdit>(FILENAME_ID)->set_text(_curr_directory.string());
}

fs::path OpenFileWindow::get_selected_file() const {
  return fs::path(wnd_->find<TextEdit>(FILENAME_ID)->get_text());
}

void OpenFileWindow::navigate_to_directory(fs::path const &new_directory) {
  _curr_directory = new_directory;
  refresh();
}

void OpenFileWindow::on_item_selected(int index) {
  fs::path item_path = _curr_directory / _items[index];
  wnd_->find<TextEdit>(FILENAME_ID)->set_text(item_path.string());

  if (fs::is_regular_file(item_path)) {
    std::string ext = item_path.extension().string();
    bool is_image = false;
    if (ext == ".jpg" || ext == ".png") {
      // we only support JPG and PNG image formats
      auto bmp_or_status = fw::load_bitmap(item_path);
      if (!bmp_or_status.ok()) {
        fw::debug << "Error loading bitmap: " << bmp_or_status.status() << std::endl;
      } else {
        auto bmp = *bmp_or_status;
        is_image = true; // if we loaded it, then we know it's an image

        Label *preview = wnd_->find<Label>(IMAGE_PREVIEW_ID);
        float width = preview->get_width();
        float height = preview->get_height();
        float bmp_width = bmp.get_width();
        float bmp_height = bmp.get_height();
        if (bmp_width > width || bmp_height > height) {
          float bmp_ratio = bmp_width / bmp_height;
          if (bmp_width > width) {
            bmp_width = width;
            bmp_height = width / bmp_ratio;
          }
          if (bmp_height > height) {
            bmp_height = height;
            bmp_width = height * bmp_ratio;
          }

          bmp.resize(bmp_width, bmp_height);
        }

        std::shared_ptr<fw::Texture> texture = std::shared_ptr<fw::Texture>(new fw::Texture());
        texture->create(bmp);
        std::shared_ptr<Drawable> drawable =
            fw::Framework::get_instance()->get_gui()->get_drawable_manager()->build_drawable(
                texture, 0, 0, bmp_width, bmp_height);
        preview->set_background(drawable, true);
      }
    }

    if (!is_image) {
      Label *preview = wnd_->find<Label>(IMAGE_PREVIEW_ID);
      preview->set_background(std::shared_ptr<Drawable>());
    }
  }
}

void OpenFileWindow::on_item_activated(int index) {
  on_ok_clicked(wnd_->find<Button>(OK_ID));
}

bool OpenFileWindow::on_show_hidden_clicked(Widget *w) {
  _show_hidden = dynamic_cast<Checkbox *>(w)->is_checked();
  refresh();
  return true;
}

bool OpenFileWindow::on_ok_clicked(Widget *w) {
  fs::path item_path = fs::path(wnd_->find<TextEdit>(FILENAME_ID)->get_text());
  if (fs::is_directory(item_path)) {
    _curr_directory = item_path;
    refresh();
    return true;
  }

  if (fs::is_regular_file(item_path)) {
    if (_file_selected_handler) {
      _file_selected_handler(this);
    }
    hide();
  }
  return true;
}

bool OpenFileWindow::on_cancel_clicked(Widget *w) {
  hide();
  return true;
}

}

std::string format_file_size(uint32_t size) {
  if (size < 1024)
    return boost::lexical_cast<std::string>(size);
  if (size < (1024 * 1024))
    return boost::lexical_cast<std::string>(size / 1024) + " KB";

  return boost::lexical_cast<std::string>(size / (1024 * 1024)) + " MB";
}

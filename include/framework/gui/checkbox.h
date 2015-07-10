#pragma once

#include <memory>
#include <string>

#include <framework/gui/widget.h>

namespace fw { namespace gui {
class gui;
class drawable;

/** Checkboxes allow you toggle settings. */
class checkbox : public widget {
public:
  enum alignment {
    left,
    center,
    right
  };

protected:
  friend class checkbox_text_property;

  std::shared_ptr<drawable> _background;
  std::shared_ptr<drawable> _check_icon;
  std::string _text;
  bool _is_mouse_over;
  bool _is_checked;

  void update_drawable_state();

public:
  checkbox(gui *gui);
  virtual ~checkbox();

  static property *text(std::string const &text);

  void on_attached_to_parent(widget *parent);
  void on_mouse_out();
  void on_mouse_over();
  bool on_mouse_down(float x, float y);
  void render();

  inline void set_text(std::string const &new_text) {
    _text = new_text;
  }
  inline std::string get_text() const {
    return _text;
  }

  void set_checked(bool is_checked);
  bool is_checked() {
    return _is_checked;
  }
};

} }

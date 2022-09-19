#pragma once

#include <memory>
#include <string>

#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/widget.h>

namespace fw::gui {

// Checkbox allow you toggle settings.
class Checkbox : public Widget {
public:
  enum Alignment {
    kLeft,
    kCenter,
    kRight
  };

protected:
  friend class CheckboxTextProperty;

  std::shared_ptr<Drawable> background_;
  std::shared_ptr<Drawable> check_icon_;
  std::string text_;
  bool is_mouse_over_;
  bool is_checked_;

  void update_drawable_state();

public:
  Checkbox(Gui *gui);
  virtual ~Checkbox();

  static Property *text(std::string const &text);

  void on_attached_to_parent(Widget *parent);
  void on_mouse_out();
  void on_mouse_over();
  bool on_mouse_down(float x, float y);
  void render();

  inline void set_text(std::string const &new_text) {
    text_ = new_text;
  }
  inline std::string get_text() const {
    return text_;
  }

  void set_checked(bool is_checked);
  bool is_checked() {
    return is_checked_;
  }
};

}

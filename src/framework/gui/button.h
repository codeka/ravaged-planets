#pragma once

#include <memory>
#include <string>

#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/widget.h>

namespace fw::gui {

/** Buttons have a background image and text (or image, or both). */
class Button : public Widget {
public:
  enum Alignment {
    kLeft,
    kCenter,
    kRight
  };

protected:
  friend class ButtonBackgroundProperty;
  friend class ButtonTextProperty;
  friend class ButtonTextAlignProperty;
  friend class ButtonIconProperty;

  std::shared_ptr<Drawable> background_;
  std::shared_ptr<Drawable> icon_;
  std::string text_;
  Alignment text_align_;
  bool is_pressed_;
  bool is_mouse_over_;

  void on_mouse_out();
  void on_mouse_over();

  void update_drawable_state();

public:
  Button(Gui *gui);
  virtual ~Button();

  static Property *background(std::string const &drawable_name);
  static Property *background(std::shared_ptr<Drawable> drawable);
  static Property *icon(std::string const &drawable_name);
  static Property *icon(std::shared_ptr<Drawable> drawable);
  static Property *text(std::string const &text);
  static Property *text_align(Alignment align);

  void on_attached_to_parent(Widget *parent);
  void render();

  inline void set_text(std::string const &new_text) {
    text_ = new_text;
  }
  inline std::string get_text() const {
    return text_;
  }
  inline void set_icon(std::shared_ptr<Drawable> drawable) {
    icon_ = drawable;
  }

  void set_pressed(bool is_pressed);
  bool is_pressed() {
    return is_pressed_;
  }
};

}
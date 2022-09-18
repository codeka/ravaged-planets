#pragma once

#include <memory>
#include <string>

#include <framework/gui/widget.h>

namespace fw::gui {
class Gui;
class Drawable;

// A Label is just a simple static control, either text or a drawable that is non-interactive.
class Label : public Widget {
public:
  enum Alignment {
    kLeft,
    kCenter,
    kRight
  };

private:
  friend class LabelBackgroundProperty;
  friend class LabelTextProperty;
  friend class LabelTextAlignProperty;

  std::string text_;
  Alignment text_alignment_;
  std::shared_ptr<Drawable> background_;
  bool background_centred_;

public:
  Label(Gui *gui);
  virtual ~Label();

  static Property *background(std::string const &drawable_name, bool centred = false);
  static Property *text(std::string const &text);
  static Property *text_align(Alignment text_alignment);

  void render();

  void set_text(std::string const &text);
  std::string get_text() const;

  void set_background(std::shared_ptr<Drawable> background, bool centred = false);
  void set_background(std::shared_ptr<bitmap> bmp, bool centred = false);
};

}

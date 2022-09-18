#pragma once

#include <memory>
#include <string>

#include <framework/gui/widget.h>

namespace fw { namespace gui {
class gui;
class drawable;

/** A label is just a simple static control, either text or a drawable that is non-interactive. */
class label : public widget {
public:
  enum alignment {
    left,
    center,
    right
  };

private:
  friend class label_background_property;
  friend class label_text_property;
  friend class label_text_align_property;

  std::string _text;
  alignment _text_alignment;
  std::shared_ptr<drawable> _background;
  bool _background_centred;

public:
  label(gui *gui);
  virtual ~label();

  static property *background(std::string const &drawable_name, bool centred = false);
  static property *text(std::string const &text);
  static property *text_align(alignment text_alignment);

  void render();

  void set_text(std::string const &text);
  std::string get_text() const;

  void set_background(std::shared_ptr<drawable> background, bool centred = false);
  void set_background(std::shared_ptr<bitmap> bmp, bool centred = false);
};

} }

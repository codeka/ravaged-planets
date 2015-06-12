#pragma once

#include <memory>
#include <string>

#include <framework/gui/property.h>
#include <framework/gui/widget.h>

namespace fw { namespace gui {
class gui;
class drawable;

/** A textedit widget is a complex widget which allows the user to type, edit, select, cut & copy text. */
class textedit : public widget {
private:
  friend class textedit_text_property;

  std::string _text;
  std::shared_ptr<drawable> _background;

public:
  textedit(gui *gui);
  virtual ~textedit();

  static property *text(std::string const &text);

  void render();
};

} }

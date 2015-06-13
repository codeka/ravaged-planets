#pragma once

#include <memory>
#include <string>

#include <framework/gui/widget.h>

class textedit_buffer;

namespace fw { namespace gui {
class gui;
class drawable;

/** A textedit widget is a complex widget which allows the user to type, edit, select, cut & copy text. */
class textedit : public widget {
private:
  friend class textedit_text_property;

  textedit_buffer *_buffer;
  std::shared_ptr<drawable> _background;
  std::shared_ptr<drawable> _selection_background;
  std::shared_ptr<drawable> _cursor;
  bool _draw_cursor;
  float _cursor_flip_time;

public:
  textedit(gui *gui);
  virtual ~textedit();

  static property *text(std::string const &text);

  virtual void on_focus_gained();
  virtual void on_focus_lost();
  virtual bool on_key(int key, bool is_down);
  virtual bool on_mouse_down(float x, float y);
  virtual bool on_mouse_up(float x, float y);

  virtual bool can_focus() const {
    return true;
  }

  void select_all();
  std::string get_text() const;
  void set_text(std::string const &text);

  virtual void update(float dt);
  virtual void render();
};

} }

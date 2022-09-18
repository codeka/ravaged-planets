#pragma once

#include <functional>
#include <memory>
#include <string>

#include <framework/gui/widget.h>

class TextEditBuffer;

namespace fw { namespace gui {
class Gui;
class Drawable;

// A TextEdit widget is a complex widget which allows the user to type, edit, select, cut & copy text.
class TextEdit : public Widget {
private:
  friend class TextEditTextProperty;
  friend class TextEditFilterProperty;

  TextEditBuffer *buffer_;
  std::shared_ptr<Drawable> background_;
  std::shared_ptr<Drawable> selection_background_;
  std::shared_ptr<Drawable> cursor_;
  std::function<bool(std::string ch)> filter_;
  bool draw_cursor_;
  float cursor_flip_time_;

public:
  TextEdit(Gui *gui);
  virtual ~TextEdit();

  static Property *text(std::string const &text);
  static Property *filter(std::function<bool(std::string ch)> filter);

  virtual void on_focus_gained();
  virtual void on_focus_lost();
  virtual bool on_key(int key, bool is_down);
  virtual bool on_mouse_down(float x, float y);
  virtual bool on_mouse_up(float x, float y);

  virtual bool can_focus() const {
    return true;
  }
  virtual std::string get_cursor_name() const;

  void select_all();
  std::string get_text() const;
  void set_text(std::string const &text);

  void set_filter(std::function<bool(std::string ch)> filter);

  virtual void update(float dt);
  virtual void render();
};

} }

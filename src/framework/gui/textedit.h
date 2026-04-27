#pragma once

#include <functional>
#include <memory>
#include <string>

#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/widget.h>

class TextEditBuffer;

namespace fw::gui {

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
  TextEdit();
  virtual ~TextEdit();

  static std::unique_ptr<Property> text(std::string_view text);
  static std::unique_ptr<Property> filter(std::function<bool(std::string ch)> filter);

  fw::Point OnMeasureSelf() override;

  void on_focus_gained() override;
  void on_focus_lost() override;
  bool on_key(int key, bool is_down) override;
  bool on_mouse_down(float x, float y) override;
  bool on_mouse_up(float x, float y) override;

  bool can_focus() const override {
    return true;
  }
  std::string get_cursor_name() const override;

  void select_all();
  std::string get_text() const;
  void set_text(std::string_view text);

  void set_filter(std::function<bool(std::string ch)> filter);

  void update(float dt) override;
  void render() override;
};

}  // namespace fw::gui

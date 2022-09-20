
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/checkbox.h>

#include <framework/framework.h>
#include <framework/font.h>


namespace fw::gui {

/** Property that sets the text of the checkbox. */
class CheckboxTextProperty : public Property {
private:
  std::string text_;
public:
  CheckboxTextProperty(std::string const &text) :
      text_(text) {
  }

  void apply(Widget *widget) {
    Checkbox *chbx = dynamic_cast<Checkbox *>(widget);
    chbx->text_ = text_;
  }
};

//-----------------------------------------------------------------------------

Checkbox::Checkbox(Gui *gui) : Widget(gui), is_checked_(false), is_mouse_over_(false) {
}

Checkbox::~Checkbox() {
}

Property * Checkbox::text(std::string const &text) {
  return new CheckboxTextProperty(text);
}

void Checkbox::on_attached_to_parent(Widget *parent) {
  StateDrawable *bkgnd = new StateDrawable();
  bkgnd->add_drawable(StateDrawable::kNormal, gui_->get_drawable_manager()->get_drawable("button_normal"));
  bkgnd->add_drawable(StateDrawable::kHover, gui_->get_drawable_manager()->get_drawable("button_hover"));
  background_ = std::shared_ptr<Drawable>(bkgnd);

  check_icon_ = gui_->get_drawable_manager()->get_drawable("checkbox");
}

void Checkbox::on_mouse_out() {
  is_mouse_over_ = false;
  update_drawable_state();
}

void Checkbox::on_mouse_over() {
  is_mouse_over_ = true;
  update_drawable_state();
}

bool Checkbox::on_mouse_down(float x, float y) {
  set_checked(!is_checked_);
  return Widget::on_mouse_down(x, y);
}

void Checkbox::set_checked(bool is_checked) {
  is_checked_ = is_checked;
  update_drawable_state();
}

void Checkbox::update_drawable_state() {
  StateDrawable *drawable = dynamic_cast<StateDrawable *>(background_.get());
  if (is_mouse_over_) {
    drawable->set_current_state(StateDrawable::kHover);
  } else {
    drawable->set_current_state(StateDrawable::kNormal);
  }
}

void Checkbox::render() {
  float left = get_left();
  float top = get_top();
  float width = get_width();
  float height = get_height();

  background_->render(left + 2, top + 2, height - 4, height - 4);

  if (is_checked_) {
    float icon_width = check_icon_->get_intrinsic_width();
    float icon_height = check_icon_->get_intrinsic_height();
    if (icon_width == 0.0f) {
      icon_width = height * 0.75f;
    }
    if (icon_height == 0.0f) {
      icon_height = height * 0.75f;
    }
    float x = left + (height / 2.0f) - (icon_width / 2.0f);
    float y = top + (height / 2.0f) - (icon_height / 2.0f);
    check_icon_->render(x, y, icon_width, icon_height);
  }

  if (text_.length() > 0) {
    fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
        left + height + 4, top + height / 2, text_,
        static_cast<fw::FontFace::DrawFlags>(fw::FontFace::kAlignLeft | fw::FontFace::kAlignMiddle));
  }
}

}

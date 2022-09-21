
#include <framework/audio.h>
#include <framework/framework.h>
#include <framework/font.h>
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/button.h>

namespace fw::gui {

// Property that sets the background of the button.
class ButtonBackgroundProperty : public Property {
private:
  std::string drawable_name_;
  std::shared_ptr<Drawable> drawable_;
public:
  ButtonBackgroundProperty(std::string const &drawable_name) :
      drawable_name_(drawable_name) {
  }
  ButtonBackgroundProperty(std::shared_ptr<Drawable> drawable) :
      drawable_(drawable) {
  }

  void apply(Widget *widget) {
    Button *btn = dynamic_cast<Button *>(widget);
    if (drawable_) {
      btn->background_ = drawable_;
    } else {
      btn->background_ = btn->gui_->get_drawable_manager()->get_drawable(drawable_name_);
    }
  }
};

// Property that sets the icon of the button.
class ButtonIconProperty : public Property {
private:
  std::string drawable_name_;
  std::shared_ptr<Drawable> drawable_;
public:
  ButtonIconProperty(std::string const &drawable_name) :
      drawable_name_(drawable_name) {
  }
  ButtonIconProperty(std::shared_ptr<Drawable> drawable) :
      drawable_(drawable) {
  }

  void apply(Widget *widget) {
    Button *btn = dynamic_cast<Button *>(widget);
    if (drawable_) {
      btn->icon_ = drawable_;
    } else {
      btn->icon_ = btn->gui_->get_drawable_manager()->get_drawable(drawable_name_);
    }
  }
};

// Property that sets the text of the button.
class ButtonTextProperty : public Property {
private:
  std::string text_;
public:
  ButtonTextProperty(std::string const &text) :
      text_(text) {
  }

  void apply(Widget *widget) {
    Button *btn = dynamic_cast<Button *>(widget);
    btn->text_ = text_;
  }
};

// Property that sets the alignment of text on the button.
class ButtonTextAlignProperty : public Property {
private:
  Button::Alignment text_align_;
public:
  ButtonTextAlignProperty(Button::Alignment text_align) :
      text_align_(text_align) {
  }

  void apply(Widget *widget) {
    Button *btn = dynamic_cast<Button *>(widget);
    btn->text_align_ = text_align_;
  }
};

//-----------------------------------------------------------------------------

static std::shared_ptr<fw::AudioBuffer> g_hover_sound;

Button::Button(Gui *gui) : Widget(gui), text_align_(kCenter), is_pressed_(false), is_mouse_over_(false) {
  sig_mouse_out.connect(std::bind(&Button::on_mouse_out, this));
  sig_mouse_over.connect(std::bind(&Button::on_mouse_over, this));

  if (!g_hover_sound) {
    g_hover_sound = fw::Framework::get_instance()->get_audio_manager()->get_audio_buffer("gui/sounds/click.ogg");
  }
}

Button::~Button() {
}

Property * Button::background(std::string const &drawable_name) {
  return new ButtonBackgroundProperty(drawable_name);
}

Property * Button::background(std::shared_ptr<Drawable> drawable) {
  return new ButtonBackgroundProperty(drawable);
}

Property * Button::icon(std::string const &drawable_name) {
  return new ButtonIconProperty(drawable_name);
}

Property * Button::icon(std::shared_ptr<Drawable> drawable) {
  return new ButtonIconProperty(drawable);
}

Property * Button::text(std::string const &text) {
  return new ButtonTextProperty(text);
}

Property * Button::text_align(Button::Alignment align) {
  return new ButtonTextAlignProperty(align);
}

void Button::on_attached_to_parent(Widget *parent) {
  // Assign default values for things that haven't been overwritten.
  if (!background_) {
    StateDrawable *bkgnd = new StateDrawable();
    bkgnd->add_drawable(StateDrawable::kNormal, gui_->get_drawable_manager()->get_drawable("button_normal"));
    bkgnd->add_drawable(StateDrawable::kHover, gui_->get_drawable_manager()->get_drawable("button_hover"));
    bkgnd->add_drawable(StateDrawable::kPressed, gui_->get_drawable_manager()->get_drawable("button_hover"));
    background_ = std::shared_ptr<Drawable>(bkgnd);
  }
}

void Button::on_mouse_out() {
  is_mouse_over_ = false;
  update_drawable_state();
}

void Button::on_mouse_over() {
  is_mouse_over_ = true;
  update_drawable_state();
}

void Button::set_pressed(bool is_pressed) {
  is_pressed_ = is_pressed;
  update_drawable_state();
}

void Button::update_drawable_state() {
  StateDrawable *drawable = dynamic_cast<StateDrawable *>(background_.get());
  if (drawable == nullptr) {
    return;
  }

  if (!is_enabled()) {
    drawable->set_current_state(StateDrawable::kDisabled);
  } else if (is_pressed_) {
    drawable->set_current_state(StateDrawable::kPressed);
  } else if (is_mouse_over_) {
    drawable->set_current_state(StateDrawable::kHover);
  } else {
    drawable->set_current_state(StateDrawable::kNormal);
  }
}

void Button::render() {
  float left = get_left();
  float top = get_top();
  float width = get_width();
  float height = get_height();

  if (background_) {
    background_->render(left, top, width, height);
  }

  if (icon_) {
    float icon_width = icon_->get_intrinsic_width();
    float icon_height = icon_->get_intrinsic_height();
    if (icon_width == 0.0f) {
      icon_width = width * 0.75f;
    }
    if (icon_height == 0.0f) {
      icon_height = height * 0.75f;
    }
    float x = left + (width / 2.0f) - (icon_width / 2.0f);
    float y = top + (height / 2.0f) - (icon_height / 2.0f);
    icon_->render(x, y, icon_width, icon_height);
  }

  if (text_.length() > 0) {
    if (text_align_ == kLeft) {
      fw::Framework::get_instance()->get_font_manager()->get_face()->draw_string(
          left + 4, top + height / 2, text_,
          static_cast<fw::FontFace::DrawFlags>(fw::FontFace::kAlignLeft | fw::FontFace::kAlignMiddle));
    } else if (text_align_ == kCenter) {
      fw::Framework::get_instance()->get_font_manager()->get_face()->draw_string(
          left + width / 2, top + height / 2, text_,
          static_cast<fw::FontFace::DrawFlags>(fw::FontFace::kAlignCenter | fw::FontFace::kAlignMiddle));
    }
  }
}

}

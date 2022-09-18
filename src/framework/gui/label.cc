
#include <framework/framework.h>
#include <framework/font.h>
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>
#include <framework/texture.h>

namespace fw::gui {

// Property that sets the background of the widget.
class LabelBackgroundProperty : public Property {
private:
  std::string drawable_name_;
  bool centred_;
public:
  LabelBackgroundProperty(std::string const &drawable_name, bool centred) :
      drawable_name_(drawable_name), centred_(centred) {
  }

  void apply(Widget *widget) {
    Label *wdgt = dynamic_cast<Label *>(widget);
    wdgt->background_ = wdgt->gui_->get_drawable_manager()->get_drawable(drawable_name_);
    wdgt->background_centred_ = centred_;
  }
};

// Property that sets the text of the widget.
class LabelTextProperty : public Property {
private:
  std::string text_;
public:
    LabelTextProperty(std::string const &text) :
    text_(text) {
  }

  void apply(Widget *widget) {
    Label *wdgt = dynamic_cast<Label *>(widget);
    wdgt->text_ = text_;
  }
};

// Property that sets the text of the widget.
class LabelTextAlignProperty : public Property {
private:
  Label::Alignment text_alignment_;
public:
    LabelTextAlignProperty(Label::Alignment text_alignment) :
    text_alignment_(text_alignment) {
  }

  void apply(Widget *widget) {
    Label *wdgt = dynamic_cast<Label *>(widget);
    wdgt->text_alignment_ = text_alignment_;
  }
};

Label::Label(Gui *gui) : Widget(gui), background_centred_(false), text_alignment_(Alignment::kLeft) {
}

Label::~Label() {
}

Property * Label::background(std::string const &drawable_name, bool centred /*= false */) {
  return new LabelBackgroundProperty(drawable_name, centred);
}

Property * Label::text(std::string const &text) {
  return new LabelTextProperty(text);
}

Property * Label::text_align(Label::Alignment text_alignment) {
  return new LabelTextAlignProperty(text_alignment);
}

void Label::render() {
  if (background_) {
    if (background_centred_) {
      float background_width = background_->get_intrinsic_width();
      float background_height = background_->get_intrinsic_height();
      if (background_width == 0.0f) {
        background_width = get_width();
      }
      if (background_height == 0.0f) {
        background_height = get_height();
      }
      float x = get_left() + (get_width() / 2.0f) - (background_width / 2.0f);
      float y = get_top() + (get_height() / 2.0f) - (background_height / 2.0f);
      background_->render(x, y, background_width, background_height);
    } else {
      background_->render(get_left(), get_top(), get_width(), get_height());
    }
  }
  if (text_ != "") {
    switch (text_alignment_) {
    case Alignment::kLeft:
      fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
        get_left(), get_top() + get_height() / 2, text_,
        static_cast<fw::font_face::draw_flags>(fw::font_face::align_left | fw::font_face::align_middle));
      break;
    case Alignment::kCenter:
      fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
        get_left() + get_width() / 2, get_top() + get_height() / 2, text_,
        static_cast<fw::font_face::draw_flags>(fw::font_face::align_centre | fw::font_face::align_middle));
      break;
    case Alignment::kRight:
      fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
          get_left() + get_width(), get_top() + get_height() / 2, text_,
          static_cast<fw::font_face::draw_flags>(fw::font_face::align_right | fw::font_face::align_middle));
      break;
    }
  }
}

void Label::set_text(std::string const &text) {
  text_ = text;
}

std::string Label::get_text() const {
  return text_;
}

void Label::set_background(std::shared_ptr<Drawable> background, bool centred /*= false */) {
  background_ = background;
  background_centred_ = centred;
}

void Label::set_background(std::shared_ptr<bitmap> bmp, bool centred /*= false*/) {
  if (!bmp) {
    background_ = nullptr;
    background_centred_ = centred;
  } else {
    std::shared_ptr<fw::texture> texture(new fw::texture());
    texture->create(bmp);
    background_ =
        gui_->get_drawable_manager()->build_drawable(texture, 0, 0, texture->get_width(), texture->get_height());
    background_centred_ = centred;
  }
}

}

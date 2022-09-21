
#include <algorithm>
#include <functional>
#include <memory>
#include <stack>
#include <vector>

#include <boost/foreach.hpp>

#include <framework/graphics.h>
#include <framework/gui/gui.h>
#include <framework/gui/widget.h>
#include <framework/misc.h>

namespace fw { namespace gui {

Dimension::Dimension() {
}

Dimension::~Dimension() {
}

PixelDimension::PixelDimension(float ParticleRotation) :
    value_(ParticleRotation) {
}

PixelDimension::~PixelDimension() {
}

float PixelDimension::get_value(fw::gui::Widget *w, float parent_value) {
  return value_;
}

PercentDimension::PercentDimension(float ParticleRotation) :
    value_(ParticleRotation) {
}

PercentDimension::~PercentDimension() {
}

float PercentDimension::get_value(fw::gui::Widget *w, float parent_value) {
  return parent_value * (value_ / 100.0f);
}

SumDimension::SumDimension(std::shared_ptr<Dimension> one, std::shared_ptr<Dimension> two) :
    one_(one), two_(two) {
}

SumDimension::~SumDimension() {
}

float SumDimension::get_value(fw::gui::Widget *w, float parent_value) {
  return one_->get_value(w, parent_value) + two_->get_value(w, parent_value);
}

FractionDimension::FractionDimension(OtherDimension dim, float fraction) :
    other_dimension_(dim), fraction_(fraction), other_widget_id_(-1) {
}

FractionDimension::FractionDimension(int other_widget_id, OtherDimension dim, float fraction) :
    other_dimension_(dim), fraction_(fraction), other_widget_id_(other_widget_id) {
}

FractionDimension::~FractionDimension() {
}

float FractionDimension::get_value(fw::gui::Widget *w, float parent_value) {
  if (other_widget_id_ >= 0) {
    w = w->get_root()->find<Widget>(other_widget_id_);
  }
  float other_value;
  switch (other_dimension_) {
  case kTop:
    other_value = w->get_top();
    break;
  case kLeft:
    other_value = w->get_left();
    break;
  case kWidth:
    other_value = w->get_width();
    break;
  case kHeight:
    other_value = w->get_height();
    break;
  }
  return other_value * fraction_;
}

//-----------------------------------------------------------------------------
class WidgetPositionProperty : public Property {
private:
  std::shared_ptr<Dimension> x_;
  std::shared_ptr<Dimension> y_;
public:
  WidgetPositionProperty(std::shared_ptr<Dimension> x, std::shared_ptr<Dimension> y);
  void apply(Widget *widget);
};

WidgetPositionProperty::WidgetPositionProperty(std::shared_ptr<Dimension> x, std::shared_ptr<Dimension> y) :
    x_(x), y_(y) {
}

void WidgetPositionProperty::apply(Widget *widget) {
  widget->x_ = x_;
  widget->y_ = y_;
}

class WidgetSizeProperty : public Property {
private:
  std::shared_ptr<Dimension> width_;
  std::shared_ptr<Dimension> height_;
public:
  WidgetSizeProperty(std::shared_ptr<Dimension> width, std::shared_ptr<Dimension> height);
  void apply(Widget *widget);
};

WidgetSizeProperty::WidgetSizeProperty(std::shared_ptr<Dimension> width, std::shared_ptr<Dimension> height) :
    width_(width), height_(height) {
}

void WidgetSizeProperty::apply(Widget *widget) {
  widget->width_ = width_;
  widget->height_ = height_;
}

class WidgetClickProperty : public Property {
private:
  std::function<bool(Widget *)> on_click_;
public:
  WidgetClickProperty(std::function<bool(Widget *)> on_click)
      : on_click_(on_click) {
  }

  void apply(Widget *widget) {
    widget->on_click_ = on_click_;
  }
};

class WidgetIdProperty : public Property {
private:
  int id_;
public:
  WidgetIdProperty(int id)
      : id_(id) {
  }

  void apply(Widget *widget) {
    widget->id_ = id_;
  }
};

class WidgetVisibleProperty : public Property {
private:
  bool visible_;
public:
  WidgetVisibleProperty(bool visible)
      : visible_(visible) {
  }

  void apply(Widget *widget) {
    widget->visible_ = visible_;
  }
};

class WidgetDataProperty : public Property {
private:
  boost::any data_;
public:
  WidgetDataProperty(boost::any const &data)
      : data_(data) {
  }

  void apply(Widget *widget) {
    widget->data_ = data_;
  }
};

class WidgetEnabledProperty : public Property {
private:
  bool enabled_;
public:
  WidgetEnabledProperty(bool enabled)
      : enabled_(enabled) {
  }

  void apply(Widget *widget) {
    widget->set_enabled(enabled_);
  }
};

//-----------------------------------------------------------------------------

Widget::Widget(Gui *gui) :
    gui_(gui), parent_(nullptr), id_(-1), visible_(true), focused_(false), enabled_(true) {
}

Widget::~Widget() {
}

Property *Widget::position(std::shared_ptr<Dimension> x, std::shared_ptr<Dimension> y) {
  return new WidgetPositionProperty(x, y);
}

Property*Widget::size(std::shared_ptr<Dimension> width, std::shared_ptr<Dimension> height) {
  return new WidgetSizeProperty(width, height);
}

Property*Widget::click(std::function<bool(Widget *)> on_click) {
  return new WidgetClickProperty(on_click);
}

Property*Widget::visible(bool visible) {
  return new WidgetVisibleProperty(visible);
}

Property*Widget::id(int id) {
  return new WidgetIdProperty(id);
}

Property*Widget::data(boost::any const &data) {
  return new WidgetDataProperty(data);
}

Property*Widget::enabled(bool enabled) {
  return new WidgetEnabledProperty(enabled);
}

void Widget::attach_child(Widget *child) {
  if (child->parent_ != nullptr) {
    child->parent_->detach_child(child);
  }
  child->parent_ = this;
  children_.push_back(child);
  child->on_attached_to_parent(this);
}

void Widget::detach_child(Widget *child) {
  children_.erase(std::find(children_.begin(), children_.end(), child));
  child->parent_ = nullptr;
}

void Widget::clear_children() {
  for(Widget *child : children_) {
    child->parent_ = nullptr;
  }
  children_.clear();
}

void Widget::on_attached_to_parent(Widget *parent) {
}

Widget *Widget::get_parent() {
  return parent_;
}

Widget *Widget::get_root() {
  if (parent_ == nullptr) {
    return this;
  }
  Widget *root = parent_;
  while (root->parent_ != nullptr) {
    root = root->parent_;
  }
  return root;
}

std::string Widget::get_cursor_name() const {
  return "arrow";
}

void Widget::on_focus_gained() {
  focused_ = true;
}

void Widget::on_focus_lost() {
  focused_ = false;
}

void Widget::update(float dt) {
  for(Widget *child : children_) {
    child->update(dt);
  }
}

std::stack<fw::Rectangle<float>> scissor_rectangles;

bool Widget::prerender() {
  fw::Rectangle<float> rect(get_left(), get_top(), get_width(), get_height());
  if (!scissor_rectangles.empty()) {
    fw::Rectangle<float> const &top = scissor_rectangles.top();
    rect = fw::Rectangle<float>::intersect(top, rect);
  }
  if (rect.width <= 0 || rect.height <= 0) {
    return false;
  }

  scissor_rectangles.push(rect);
  FW_CHECKED(glScissor(rect.left, gui_->get_height() - rect.top - rect.height, rect.width, rect.height));
  return true;
}

void Widget::render() {
  for(Widget *child : children_) {
    if (child->is_visible() && child->prerender()) {
      child->render();
      child->postrender();
    }
  }
}

void Widget::postrender() {
  scissor_rectangles.pop();
  if (!scissor_rectangles.empty()) {
    fw::Rectangle<float> const &top = scissor_rectangles.top();
    FW_CHECKED(glScissor(top.left, gui_->get_height() - top.top - top.height, top.width, top.height));
  }
}

bool Widget::on_mouse_down(float x, float y) {
  return enabled_ && on_click_ != nullptr;
}

bool Widget::on_mouse_up(float x, float y) {
  if (enabled_ && on_click_) {
    return on_click_(this);
  }
  return false;
}

Widget *Widget::get_child_at(float x, float y) {
  float left = get_left();
  float top = get_top();
  float right = left + get_width();
  float bottom = top + get_height();

  // If we're outside the given (x,y) then return null.
  if (x < left || y < top || x >= right || y >= bottom) {
    return nullptr;
  }

  // If one of our children is within the (x,y) then return that.
  for(Widget *child : children_) {
    Widget *found = child->get_child_at(x, y);
    if (found != nullptr) {
      return found;
    }
  }

  // Otherwise, return ourselves.
  return this;
}

Widget *Widget::find(int id) {
  if (id_ == id) {
    return this;
  }
  for(Widget *child : children_) {
    Widget *found = child->find(id);
    if (found != nullptr) {
      return found;
    }
  }
  return nullptr;
}

boost::any const &Widget::get_data() const {
  return data_;
}

void Widget::set_data(boost::any const &data) {
  data_ = data;
}

bool Widget::is_child(Widget *w) {
  if (w == nullptr) {
    return false;
  }

  for(Widget *child : children_) {
    if (w == child) {
      return true;
    } else if (child->is_child(w)) {
      return true;
    }
  }

  return false;
}

void Widget::set_visible(bool visible) {
  visible_ = visible;
  if (parent_ == nullptr) {
    // if it's a top-level widget, move it to the front of the z-order
    gui_->bring_to_top(this);
  }
}

void Widget::set_enabled(bool enabled) {
  enabled_ = enabled;
}

float Widget::get_top() {
  float parent_top = (parent_ != nullptr) ? parent_->get_top() : 0;
  float parent_size = (parent_ != nullptr) ? parent_->get_height() : gui_->get_height();
  return parent_top + y_->get_value(this, parent_size);
}

void Widget::set_top(std::shared_ptr<Dimension> top) {
  y_ = top;
}

float Widget::get_left() {
  float parent_left = (parent_ != nullptr) ? parent_->get_left() : 0;
  float parent_size = (parent_ != nullptr) ? parent_->get_width() : gui_->get_width();
  return parent_left + x_->get_value(this, parent_size);
}

void Widget::set_left(std::shared_ptr<Dimension> left) {
  x_ = left;
}

float Widget::get_width() {
  float parent_size = (parent_ != nullptr) ? parent_->get_width() : gui_->get_width();
  return width_->get_value(this, parent_size);
}

void Widget::set_width(std::shared_ptr<Dimension> width) {
  width_ = width;
}

float Widget::get_height() {
  float parent_size = (parent_ != nullptr) ? parent_->get_height() : gui_->get_height();
  return height_->get_value(this, parent_size);
}

void Widget::set_height(std::shared_ptr<Dimension> height) {
  height_ = height;
}

} }

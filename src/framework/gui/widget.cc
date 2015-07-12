
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

dimension::dimension() {
}

dimension::~dimension() {
}

pixel_dimension::pixel_dimension(float value) :
    _value(value) {
}

pixel_dimension::~pixel_dimension() {
}

float pixel_dimension::get_value(fw::gui::widget *w, float parent_value) {
  return _value;
}

percent_dimension::percent_dimension(float value) :
    _value(value) {
}

percent_dimension::~percent_dimension() {
}

float percent_dimension::get_value(fw::gui::widget *w, float parent_value) {
  return parent_value * (_value / 100.0f);
}

sum_dimension::sum_dimension(std::shared_ptr<dimension> one, std::shared_ptr<dimension> two) :
    _one(one), _two(two) {
}

sum_dimension::~sum_dimension() {
}

float sum_dimension::get_value(fw::gui::widget *w, float parent_value) {
  return _one->get_value(w, parent_value) + _two->get_value(w, parent_value);
}

fraction_dimension::fraction_dimension(other_dimension dim, float fraction) :
    _other_dimension(dim), _fraction(fraction) {
}

fraction_dimension::~fraction_dimension() {
}

float fraction_dimension::get_value(fw::gui::widget *w, float parent_value) {
  float other_value;
  switch (_other_dimension) {
  case top:
    other_value = w->get_top();
    break;
  case left:
    other_value = w->get_left();
    break;
  case width:
    other_value = w->get_width();
    break;
  case height:
    other_value = w->get_height();
    break;
  }
  return other_value * _fraction;
}

//-----------------------------------------------------------------------------
class widget_position_property : public property {
private:
  std::shared_ptr<dimension> _x;
  std::shared_ptr<dimension> _y;
public:
  widget_position_property(std::shared_ptr<dimension> x, std::shared_ptr<dimension> y);
  void apply(widget *widget);
};

widget_position_property::widget_position_property(std::shared_ptr<dimension> x, std::shared_ptr<dimension> y) :
    _x(x), _y(y) {
}

void widget_position_property::apply(widget *widget) {
  widget->_x = _x;
  widget->_y = _y;
}

class widget_size_property : public property {
private:
  std::shared_ptr<dimension> _width;
  std::shared_ptr<dimension> _height;
public:
  widget_size_property(std::shared_ptr<dimension> width, std::shared_ptr<dimension> height);
  void apply(widget *widget);
};

widget_size_property::widget_size_property(std::shared_ptr<dimension> width, std::shared_ptr<dimension> height) :
    _width(width), _height(height) {
}

void widget_size_property::apply(widget *widget) {
  widget->_width = _width;
  widget->_height = _height;
}

class widget_click_property : public property {
private:
  std::function<bool(widget *)> _on_click;
public:
  widget_click_property(std::function<bool(widget *)> on_click)
      : _on_click(on_click) {
  }

  void apply(widget *widget) {
    widget->_on_click = _on_click;
  }
};

class widget_id_property : public property {
private:
  int _id;
public:
  widget_id_property(int id)
      : _id(id) {
  }

  void apply(widget *widget) {
    widget->_id = _id;
  }
};

class widget_visible_property : public property {
private:
  bool _visible;
public:
  widget_visible_property(bool visible)
      : _visible(visible) {
  }

  void apply(widget *widget) {
    widget->_visible = _visible;
  }
};

class widget_data_property : public property {
private:
  boost::any _data;
public:
  widget_data_property(boost::any const &data)
      : _data(data) {
  }

  void apply(widget *widget) {
    widget->_data = _data;
  }
};

//-----------------------------------------------------------------------------

widget::widget(gui *gui) :
    _gui(gui), _parent(nullptr), _id(-1), _visible(true), _focused(false) {
}

widget::~widget() {
}

property *widget::position(std::shared_ptr<dimension> x, std::shared_ptr<dimension> y) {
  return new widget_position_property(x, y);
}

property *widget::size(std::shared_ptr<dimension> width, std::shared_ptr<dimension> height) {
  return new widget_size_property(width, height);
}

property *widget::click(std::function<bool(widget *)> on_click) {
  return new widget_click_property(on_click);
}

property *widget::visible(bool visible) {
  return new widget_visible_property(visible);
}

property *widget::id(int id) {
  return new widget_id_property(id);
}

property *widget::data(boost::any const &data) {
  return new widget_data_property(data);
}

void widget::attach_child(widget *child) {
  if (child->_parent != nullptr) {
    child->_parent->detach_child(child);
  }
  child->_parent = this;
  _children.push_back(child);
  child->on_attached_to_parent(this);
}

void widget::detach_child(widget *child) {
  _children.erase(std::find(_children.begin(), _children.end(), child));
  child->_parent = nullptr;
}

void widget::clear_children() {
  BOOST_FOREACH(widget *child, _children) {
    child->_parent = nullptr;
  }
  _children.clear();
}

void widget::on_attached_to_parent(widget *parent) {
}

widget *widget::get_parent() {
  return _parent;
}

void widget::on_focus_gained() {
  _focused = true;
}

void widget::on_focus_lost() {
  _focused = false;
}

void widget::update(float dt) {
  BOOST_FOREACH(widget *child, _children) {
    child->update(dt);
  }
}

std::stack<fw::rectangle<float>> scissor_rectangles;

bool widget::prerender() {
  fw::rectangle<float> rect(get_left(), get_top(), get_width(), get_height());
  if (!scissor_rectangles.empty()) {
    fw::rectangle<float> const &top = scissor_rectangles.top();
    rect = fw::rectangle<float>::intersect(top, rect);
  }
  if (rect.width <= 0 || rect.height <= 0) {
    return false;
  }

  scissor_rectangles.push(rect);
  FW_CHECKED(glScissor(rect.left, _gui->get_height() - rect.top - rect.height, rect.width, rect.height));
  return true;
}

void widget::render() {
  BOOST_FOREACH(widget *child, _children) {
    if (child->is_visible() && child->prerender()) {
      child->render();
      child->postrender();
    }
  }
}

void widget::postrender() {
  scissor_rectangles.pop();
  if (!scissor_rectangles.empty()) {
    fw::rectangle<float> const &top = scissor_rectangles.top();
    FW_CHECKED(glScissor(top.left, _gui->get_height() - top.top - top.height, top.width, top.height));
  }
}

bool widget::on_mouse_down(float x, float y) {
  return _on_click != nullptr;
}

bool widget::on_mouse_up(float x, float y) {
  if (_on_click) {
    return _on_click(this);
  }
  return false;
}

widget *widget::get_child_at(float x, float y) {
  float left = get_left();
  float top = get_top();
  float right = left + get_width();
  float bottom = top + get_height();

  // If we're outside the given (x,y) then return null.
  if (x < left || y < top || x >= right || y >= bottom) {
    return nullptr;
  }

  // If one of our children is within the (x,y) then return that.
  BOOST_FOREACH(widget *child, _children) {
    widget *found = child->get_child_at(x, y);
    if (found != nullptr) {
      return found;
    }
  }

  // Otherwise, return ourselves.
  return this;
}

widget *widget::find(int id) {
  if (_id == id) {
    return this;
  }
  BOOST_FOREACH(widget *child, _children) {
    widget *found = child->find(id);
    if (found != nullptr) {
      return found;
    }
  }
  return nullptr;
}

boost::any const &widget::get_data() const {
  return _data;
}

void widget::set_data(boost::any const &data) {
  _data = data;
}

bool widget::is_child(widget *w) {
  if (w == nullptr) {
    return false;
  }

  BOOST_FOREACH(widget *child, _children) {
    if (w == child) {
      return true;
    } else if (child->is_child(w)) {
      return true;
    }
  }

  return false;
}

void widget::set_visible(bool visible) {
  _visible = visible;
  if (_parent == nullptr) {
    // if it's a top-level widget, move it to the front of the z-order
    _gui->bring_to_top(this);
  }
}

float widget::get_top() {
  float parent_top = (_parent != nullptr) ? _parent->get_top() : 0;
  float parent_size = (_parent != nullptr) ? _parent->get_height() : _gui->get_height();
  return parent_top + _y->get_value(this, parent_size);
}

void widget::set_top(std::shared_ptr<dimension> top) {
  _y = top;
}

float widget::get_left() {
  float parent_left = (_parent != nullptr) ? _parent->get_left() : 0;
  float parent_size = (_parent != nullptr) ? _parent->get_width() : _gui->get_width();
  return parent_left + _x->get_value(this, parent_size);
}

void widget::set_left(std::shared_ptr<dimension> left) {
  _x = left;
}

float widget::get_width() {
  float parent_size = (_parent != nullptr) ? _parent->get_width() : _gui->get_width();
  return _width->get_value(this, parent_size);
}

void widget::set_width(std::shared_ptr<dimension> width) {
  _width = width;
}

float widget::get_height() {
  float parent_size = (_parent != nullptr) ? _parent->get_height() : _gui->get_height();
  return _height->get_value(this, parent_size);
}

void widget::set_height(std::shared_ptr<dimension> height) {
  _height = height;
}

} }

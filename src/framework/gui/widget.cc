
#include <algorithm>

#include <boost/foreach.hpp>

#include <framework/gui/gui.h>
#include <framework/gui/widget.h>

namespace fw { namespace gui {

dimension::dimension() :
    _kind(pixels), _value(0) {
}

dimension::dimension(kind kind, float value) :
    _kind(kind), _value(value) {
}

//-----------------------------------------------------------------------------

position_property::position_property(dimension const &x, dimension const &y) :
    _x(x), _y(y) {
}

void position_property::apply(widget *widget) {
  widget->_x = _x;
  widget->_y = _y;
}

size_property::size_property(dimension const &width, dimension const &height) :
    _width(width), _height(height) {
}

void size_property::apply(widget *widget) {
  widget->_width = _width;
  widget->_height = _height;
}

//-----------------------------------------------------------------------------

widget::widget(gui *gui) :
    _gui(gui), _parent(nullptr) {
}

widget::~widget() {
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

void widget::on_attached_to_parent(widget *parent) {
}

void widget::render() {
  BOOST_FOREACH(widget *child, _children) {
    child->render();
  }
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

float widget::get_top() {
  if (_y._kind == dimension::percent) {
    float parent_size = (_parent != nullptr) ? _parent->get_height() : _gui->get_height();
    return parent_size * _y._value / 100.0f;
  } else {
    return _y._value;
  }
}

float widget::get_left() {
  if (_x._kind == dimension::percent) {
    float parent_size = (_parent != nullptr) ? _parent->get_width() : _gui->get_width();
    return parent_size * _x._value / 100.0f;
  } else {
    return _x._value;
  }
}

float widget::get_width() {
  if (_width._kind == dimension::percent) {
    float parent_size = (_parent != nullptr) ? _parent->get_width() : _gui->get_width();
    return parent_size * _width._value / 100.0f;
  } else {
    return _width._value;
  }
}

float widget::get_height() {
  if (_height._kind == dimension::percent) {
    float parent_size = (_parent != nullptr) ? _parent->get_height() : _gui->get_height();
    return parent_size * _height._value / 100.0f;
  } else {
    return _height._value;
  }
}

} }

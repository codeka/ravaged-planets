
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

widget::widget(gui *gui) : _gui(gui) {
}

widget::~widget() {
}

float widget::get_top() {
  if (_y._kind == dimension::percent) {
    // TODO: look at parent
    return _gui->get_height() * _y._value / 100.0f;
  } else {
    return _y._value;
  }
}

float widget::get_left() {
  if (_x._kind == dimension::percent) {
    // TODO: look at parent
    return _gui->get_width() * _x._value / 100.0f;
  } else {
    return _x._value;
  }
}

float widget::get_width() {
  if (_width._kind == dimension::percent) {
    // TODO: look at parent
    return _gui->get_width() * _width._value / 100.0f;
  } else {
    return _width._value;
  }
}

float widget::get_height() {
  if (_height._kind == dimension::percent) {
    // TODO: look at parent
    return _gui->get_height() * _height._value / 100.0f;
  } else {
    return _height._value;
  }
}

} }


#include <framework/gui/widget.h>

namespace fw { namespace gui {

dimension::dimension() :
    _kind(pixels), _value(0) {
}

dimension::dimension(kind kind, float value) :
    _kind(kind), _value(value) {
}

float dimension::resolve(widget *widget) {
  // TODO: actually resolve.
  return _value;
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

widget::widget() {
}

widget::~widget() {
}

} }

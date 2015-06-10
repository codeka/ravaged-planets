#pragma once

#include <memory>
#include <boost/foreach.hpp>

#include <framework/framework.h>

#include <framework/gui/widget.h>
#include <framework/gui/window.h>
#include <framework/gui/property.h>

namespace fw { namespace gui {

/**
 * This class can be used to build other GUI classes. It keeps a list of attribute definitions and child elements
 * and when you call build() will actually build the class, passing in all those things.
 */
template <class widget_type>
class builder {
private:
  std::vector<property *> _properties;
  std::vector<widget *> _child_widgets;

public:
  inline builder();
  // Helper constructor which auto-adds a position and size property for the specified (x, y) and (width, height).
  inline builder(std::shared_ptr<dimension> x, std::shared_ptr<dimension> y, std::shared_ptr<dimension> width,
      std::shared_ptr<dimension> height);
  inline ~builder();

  inline operator widget_type *();
  inline builder &operator <<(property *prop);
  inline builder &operator <<(widget *child);
};

template<class widget_type>
inline builder<widget_type>::builder() {
}

template<class widget_type>
inline builder<widget_type>::builder(std::shared_ptr<dimension> x, std::shared_ptr<dimension> y,
    std::shared_ptr<dimension> width, std::shared_ptr<dimension> height) {
  _properties.push_back(widget::position(x, y));
  _properties.push_back(widget::size(width, height));
}

template<class widget_type>
inline builder<widget_type>::~builder() {
  BOOST_FOREACH(property *prop, _properties) {
    delete prop;
  }
}

template<class widget_type>
inline builder<widget_type>::operator widget_type *() {
  widget_type *new_widget = new widget_type(fw::framework::get_instance()->get_gui());
  BOOST_FOREACH(property *prop, _properties) {
    prop->apply(new_widget);
  }
  BOOST_FOREACH(widget *child, _child_widgets) {
    new_widget->attach_child(child);
  }
  return new_widget;
}

template<class widget_type>
inline builder<widget_type> &builder<widget_type>::operator <<(property *prop) {
  _properties.push_back(prop);
  return *this;
}

template<class widget_type>
inline builder<widget_type> &builder<widget_type>::operator <<(widget *child) {
  _child_widgets.push_back(child);
  return *this;
}

} }

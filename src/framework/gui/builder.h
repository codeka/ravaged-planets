#pragma once

#include <memory>
#include <boost/foreach.hpp>

#include <framework/framework.h>

#include <framework/gui/widget.h>
#include <framework/gui/window.h>

namespace fw::gui {

// This class can be used to build other GUI classes. It keeps a list of attribute definitions and child elements
// and when you call build() will actually build the class, passing in all those things.
template <class WidgetType>
class Builder {
private:
  std::vector<Property *> properties_;
  std::vector<Widget *> child_widgets_;

public:
  inline Builder();
  // Helper constructor which auto-adds a position and size property for the specified (x, y) and (width, height).
  inline Builder(std::shared_ptr<Dimension> x, std::shared_ptr<Dimension> y, std::shared_ptr<Dimension> width,
      std::shared_ptr<Dimension> height);
  inline ~Builder();

  inline operator WidgetType *();
  inline Builder &operator <<(Property *prop);
  inline Builder &operator <<(Widget *child);
};

template<class WidgetType>
inline Builder<WidgetType>::Builder() {
}

template<class WidgetType>
inline Builder<WidgetType>::Builder(std::shared_ptr<Dimension> x, std::shared_ptr<Dimension> y,
    std::shared_ptr<Dimension> width, std::shared_ptr<Dimension> height) {
  properties_.push_back(Widget::position(x, y));
  properties_.push_back(Widget::size(width, height));
}

template<class WidgetType>
inline Builder<WidgetType>::~Builder() {
  for (Property *prop : properties_) {
    delete prop;
  }
}

template<class WidgetType>
inline Builder<WidgetType>::operator WidgetType*() {
  WidgetType *new_widget = new WidgetType(fw::framework::get_instance()->get_gui());
  for (Property *prop : properties_) {
    prop->apply(new_widget);
  }
  for (Widget *child : child_widgets_) {
    new_widget->attach_child(child);
  }
  return new_widget;
}

template<class WidgetType>
inline Builder<WidgetType> & Builder<WidgetType>::operator <<(Property *prop) {
  properties_.push_back(prop);
  return *this;
}

template<class WidgetType>
inline Builder<WidgetType> & Builder<WidgetType>::operator <<(Widget *child) {
  child_widgets_.push_back(child);
  return *this;
}

}

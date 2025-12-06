#pragma once

#include <memory>

#include <framework/framework.h>

#include <framework/gui/widget.h>
#include <framework/gui/window.h>

namespace fw::gui {

// This class can be used to build other GUI classes. It keeps a list of attribute definitions and child elements
// and when you call build() will actually build the class, passing in all those things.
template <class WidgetType>
class Builder {
private:
  std::vector<std::unique_ptr<Property>> properties_;
  std::vector<Widget *> child_widgets_;

public:
  inline Builder();
  // Helper constructor which auto-adds a position and size property for the specified (x, y) and
  // (width, height).
  inline Builder(
      std::unique_ptr<Dimension> x, std::unique_ptr<Dimension> y, std::unique_ptr<Dimension> width,
      std::unique_ptr<Dimension> height);
  inline ~Builder() = default;

  inline operator WidgetType *();
  inline Builder &operator <<(Property *prop);
  inline Builder &operator <<(std::unique_ptr<Property> prop);
  inline Builder &operator <<(Widget *child);
};

template<class WidgetType>
inline Builder<WidgetType>::Builder() {
}

template<class WidgetType>
inline Builder<WidgetType>::Builder(std::unique_ptr<Dimension> x, std::unique_ptr<Dimension> y,
    std::unique_ptr<Dimension> width, std::unique_ptr<Dimension> height) {
  properties_.push_back(std::move(Widget::position(std::move(x), std::move(y))));
  properties_.push_back(std::move(Widget::size(std::move(width), std::move(height))));
}

template<class WidgetType>
inline Builder<WidgetType>::operator WidgetType*() {
  WidgetType *new_widget = new WidgetType(fw::Framework::get_instance()->get_gui());
  for (auto &prop : properties_) {
    prop->apply(new_widget);
  }
  for (Widget *child : child_widgets_) {
    new_widget->attach_child(child);
  }
  return new_widget;
}

template<class WidgetType>
inline Builder<WidgetType> & Builder<WidgetType>::operator <<(Property *prop) {
  properties_.push_back(std::unique_ptr<Property>(prop));
  return *this;
}

template<class WidgetType>
inline Builder<WidgetType> & Builder<WidgetType>::operator <<(std::unique_ptr<Property> prop) {
  properties_.push_back(std::move(prop));
  return *this;
}

template<class WidgetType>
inline Builder<WidgetType> & Builder<WidgetType>::operator <<(Widget *child) {
  child_widgets_.push_back(child);
  return *this;
}

}

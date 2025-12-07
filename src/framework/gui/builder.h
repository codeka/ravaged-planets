#pragma once

#include <memory>

#include <framework/framework.h>

#include <framework/gui/widget.h>
#include <framework/gui/window.h>

namespace fw::gui {

// This class can be used to build other GUI classes. It keeps a list of attribute definitions and
// child elements and when you call build() will actually build the class, passing in all those
// things.
template <class WidgetType>
class Builder {
private:
  std::vector<std::unique_ptr<Property>> properties_;
  std::vector<std::shared_ptr<Widget>> child_widgets_;

public:
  inline Builder();
  // Helper constructor which auto-adds a position and size property for the specified (x, y) and
  // (width, height).
  inline Builder(
      std::unique_ptr<Dimension> x, std::unique_ptr<Dimension> y, std::unique_ptr<Dimension> width,
      std::unique_ptr<Dimension> height);
  inline ~Builder() = default;

  inline operator std::shared_ptr<WidgetType>();
  inline Builder &operator <<(std::unique_ptr<Property> prop);
  inline Builder &operator <<(std::shared_ptr<Widget> child);

  template <IsSubclassOfWidget T>
  inline Builder &operator <<(Builder<T> &child_builder) {
    child_widgets_.push_back(std::dynamic_pointer_cast<Widget>(std::shared_ptr<T>(child_builder)));
    return *this;
  }
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
inline Builder<WidgetType>::operator std::shared_ptr<WidgetType>() {
  auto new_widget = std::make_shared<WidgetType>(fw::Framework::get_instance()->get_gui());
  for (auto &prop : properties_) {
    prop->apply(*new_widget);
  }
  for (auto &child : child_widgets_) {
    new_widget->AttachChild(child);
  }
  return new_widget;
}

template<class WidgetType>
inline Builder<WidgetType> & Builder<WidgetType>::operator <<(std::unique_ptr<Property> prop) {
  properties_.push_back(std::move(prop));
  return *this;
}

template<class WidgetType>
inline Builder<WidgetType> & Builder<WidgetType>::operator <<(std::shared_ptr<Widget> child) {
  child_widgets_.push_back(child);
  return *this;
}

template<IsSubclassOfWidget T>
void Widget::AttachChild(Builder<T> &child_builder) {
  AttachChild(std::dynamic_pointer_cast<Widget>(std::shared_ptr<T>(child_builder)));
}

}

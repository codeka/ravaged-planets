#pragma once

#include <memory>

#include <framework/framework.h>

#include <framework/gui/widget.h>
#include <framework/gui/window.h>

namespace fw::gui {

class RealBuilder {
 private:
  std::function<std::shared_ptr<Widget>()> create_func_;
 public:
  std::vector<std::unique_ptr<Property>> properties;
	std::vector<std::shared_ptr<RealBuilder>> child_builders;

  std::shared_ptr<Widget> Build(std::shared_ptr<Widget> parent);

  explicit RealBuilder(std::function<std::shared_ptr<Widget>()> create_func)
    : create_func_(create_func) {}
  virtual ~RealBuilder() = default;
};

inline std::shared_ptr<Widget> RealBuilder::Build(std::shared_ptr<Widget> parent) {
  auto new_widget = create_func_();
  if (parent != nullptr) {
    parent->AttachChild(new_widget);
  }
  for (auto &child : child_builders) {
    child->Build(new_widget);
  }
  for (auto& prop : properties) {
    prop->apply(*new_widget);
  }
  return new_widget;
}

// This class can be used to build other GUI classes. It keeps a list of attribute definitions and
// child elements and when you call build() will actually build the class, passing in all those
// things.
template <class WidgetType>
class Builder {
public:
  std::shared_ptr<RealBuilder> real_builder;

  inline Builder() {
    real_builder = std::make_shared<RealBuilder>([]() {
        return std::make_shared<WidgetType>();
			});
  }
  inline ~Builder() = default;

  inline operator std::shared_ptr<WidgetType>() {
    return std::dynamic_pointer_cast<WidgetType>(real_builder->Build(nullptr));
  }

  inline std::shared_ptr<WidgetType> Build(std::shared_ptr<Widget> parent) {
    return std::dynamic_pointer_cast<WidgetType>(real_builder->Build(parent));
	}

  inline Builder& operator <<(std::unique_ptr<Property> prop) {
    real_builder->properties.push_back(std::move(prop));
    return *this;
  }

  template <IsSubclassOfWidget T>
  inline Builder &operator <<(Builder<T> &child_builder) {
    real_builder->child_builders.push_back(child_builder.real_builder);
    return *this;
  }
};

template<IsSubclassOfWidget T>
void Widget::AttachChild(Builder<T> &child_builder) {
  AttachChild(child_builder.real_builder->Build(this->shared_from_this()));
}

}

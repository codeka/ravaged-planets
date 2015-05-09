#pragma once

#include <boost/foreach.hpp>

#include <framework/framework.h>

namespace fw { namespace gui {

/** Base class for properties that can be added to buildable objects. */
class property {
public:
  inline virtual ~property() { }

  virtual void apply(window *wnd) = 0;
};

/**
 * This class can be used to build other GUI classes. It keeps a list of attribute definitions and child elements
 * and when you call build() will actually build the class, passing in all those things.
 */
template <class buildable>
class builder {
private:
  std::vector<property *> _properties;

public:
  inline operator std::shared_ptr<buildable>();

  inline builder &operator <<(property *prop);
};

template<>
inline builder<window>::operator std::shared_ptr<window>() {
  fw::gui::window *wnd = fw::framework::get_instance()->get_gui()->create_window();
  BOOST_FOREACH(property *prop, _properties) {
    prop->apply(wnd);
  }
  return std::shared_ptr<window>(wnd);
}

template<class buildable>
inline builder<buildable>::operator std::shared_ptr<buildable>() {
  return new buildable();
}

template<class buildable>
inline builder<buildable> &builder<buildable>::operator <<(property *prop) {
  _properties.push_back(prop);
  return *this;
}

} }

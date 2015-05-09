#pragma once

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
template <class buildable>
class builder {
private:
  std::vector<property *> _properties;

public:
  inline builder();
  inline ~builder();

  inline operator std::shared_ptr<buildable>();
  inline builder &operator <<(property *prop);
};

template<class buildable>
inline builder<buildable>::builder() {
}

template<class buildable>
inline builder<buildable>::~builder() {
  BOOST_FOREACH(property *prop, _properties) {
    delete prop;
  }
}

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

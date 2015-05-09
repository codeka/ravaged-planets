#pragma once

namespace fw { namespace gui {
class widget;

/** Base class for properties that can be added to buildable objects. */
class property {
public:
  inline virtual ~property() { }

  virtual void apply(widget *widget) = 0;
};

} }

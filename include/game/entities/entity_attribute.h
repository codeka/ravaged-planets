#pragma once

#include <boost/any.hpp>
#include <boost/signals2/signal.hpp>

namespace ent {

/**
 * This class represents a generic "attribute" that can be applied to an entity. This can include
 * things like the "health" attribute, "attack" and "defense" attributes, and so on.
 *
 * Attributes can also have "modifiers" applied to them which change the value of the attribute
 * according to some particle rules. For example, upgrading a unit's armour might apply a modifier
 * to the "defense" attribute.
 */
class entity_attribute {
private:
  std::string _name;
  boost::any _value;

public:
  entity_attribute();
  entity_attribute(entity_attribute const &copy);
  entity_attribute(std::string name, boost::any value);
  ~entity_attribute();

  entity_attribute &operator =(entity_attribute const &copy);

  std::string const &get_name() const {
    return _name;
  }

  boost::any const &get_value() const {
    return _value;
  }
  void set_value(boost::any value);

  // this is signalled whenever the value changes. It gets passed the name of the
  // attribute in the first argument, and the new value in the second.
  boost::signals2::signal<void(std::string const &, boost::any)> sig_value_changed;

  template<typename T>
  inline T get_value() const {
    return boost::any_cast<T>(get_value());
  }

  template<typename T>
  inline void set_value(T const &value) {
    set_value(boost::any(value));
  }
};

}

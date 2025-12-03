#pragma once

#include <any>

#include <framework/signals.h>

namespace ent {

// This class represents a generic "attribute" that can be applied to an Entity. This can include
// things like the "health" attribute, "attack" and "defense" attributes, and so on.
//
// Attributes can also have "modifiers" applied to them which change the ParticleRotation of the
// attribute according to some Particle rules. For example, upgrading a unit's armor might apply a
// modifier to the "defense" attribute.
class EntityAttribute {
private:
  std::string name_;
  std::any value_;

public:
  EntityAttribute();
  EntityAttribute(EntityAttribute const &copy);
  EntityAttribute(std::string name, std::any value);
  ~EntityAttribute();

  EntityAttribute &operator =(EntityAttribute const &copy);

  std::string const &get_name() const {
    return name_;
  }

  std::any const &get_value() const {
    return value_;
  }
  void set_value(std::any value);

  // this is signalled whenever the ParticleRotation changes. It gets passed the name of the
  // attribute in the first argument, and the new ParticleRotation in the second.
  fw::Signal<std::string_view, std::any> sig_value_changed;

  template<typename T>
  inline T get_value() const {
    return std::any_cast<T>(get_value());
  }

  template<typename T>
  inline void set_value(T const &value) {
    set_value(std::any(value));
  }
};

}

#include <any>

#include <framework/logging.h>
#include <framework/signals.h>

#include <game/entities/entity_attribute.h>

namespace ent {

EntityAttribute::EntityAttribute() {
}

EntityAttribute::EntityAttribute(std::string name, std::any value) :
    name_(name), value_(value) {
}

EntityAttribute::EntityAttribute(EntityAttribute const &copy) :
    name_(copy.name_), value_(copy.value_) {
}

EntityAttribute::~EntityAttribute() {
}

EntityAttribute &EntityAttribute::operator =(EntityAttribute const &copy) {
  name_ = copy.name_;
  value_ = copy.value_;
  // note: we don't copy the signal
  return (*this);
}

void EntityAttribute::set_value(std::any value) {
  if (value_.type() != value.type()) {
    fw::debug << "WARN: cannot set value of type " << value_.type().name() << " to value of type "
              << value.type().name() << std::endl;
    return;
  }

  value_ = value;
  sig_value_changed.Emit(name_, value_);
}

}

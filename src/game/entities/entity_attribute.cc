#include <boost/format.hpp>

#include <framework/logging.h>
#include <game/entities/entity_attribute.h>

namespace ent {

EntityAttribute::EntityAttribute() {
}

EntityAttribute::EntityAttribute(std::string name, boost::any value) :
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

void EntityAttribute::set_value(boost::any value) {
  if (value_.type() != value.type()) {
    fw::debug << boost::format("WARN: cannot set value of type %1% to value of type %2%")
        % value_.type().name() % value.type().name() << std::endl;
    return;
  }

  value_ = value;
  sig_value_changed(name_, value_);
}

}

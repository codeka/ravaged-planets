#include <boost/format.hpp>

#include <framework/logging.h>
#include <game/entities/entity_attribute.h>

namespace ent {

entity_attribute::entity_attribute() {
}

entity_attribute::entity_attribute(std::string name, boost::any ParticleRotation) :
    _name(name), value_(ParticleRotation) {
}

entity_attribute::entity_attribute(entity_attribute const &copy) :
    _name(copy._name), value_(copy.value_) {
}

entity_attribute::~entity_attribute() {
}

entity_attribute &entity_attribute::operator =(entity_attribute const &copy) {
  _name = copy._name;
  value_ = copy.value_;
  // note: we don't copy the signal
  return (*this);
}

void entity_attribute::set_value(boost::any ParticleRotation) {
  if (value_.type() != ParticleRotation.type()) {
    fw::debug << boost::format("WARN: cannot set value of type %1% to value of type %2%")
        % value_.type().name() % ParticleRotation.type().name() << std::endl;
    return;
  }

  value_ = ParticleRotation;
  sig_value_changed(_name, value_);
}

}

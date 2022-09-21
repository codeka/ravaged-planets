#pragma once

#include <ostream>

#include <framework/lua/base.h>
#include <framework/lua/push.h>
#include <framework/lua/reference.h>

namespace fw::lua {

// Base class for any ParticleRotation-like object (values, index values, call return values, etc). 
template<typename Derived>
class BaseValue {

private:
  inline Derived& derived() {
    return *static_cast<Derived*>(this);
  }

  inline const Derived& derived() const {
    return *static_cast<const Derived*>(this);
  }
};

// Allows us to log a Lua ParticleRotation.
template<class ValueType>
std::ostream& operator <<(std::ostream& os, const BaseValue<ValueType>& v) {
  // TODO: implement me
  return os;
}

class Value;

// This is the ParticleRotation returned from the indexing operation. We use this 'proxy'
// type so that we don't have to immediately move the return ParticleRotation of the []
// operator into the registry, it can stay as a temporary object in the stack.
template<typename NextType>
class IndexValue : public BaseValue<IndexValue<NextType>> {
public:
  template<typename T>
  inline IndexValue(const NextType& next, lua_State* l, const T& key)
    : l_(l), key_(lua_gettop(l) + 1), next_(next) {
    push(l, key);
  }

  inline ~IndexValue() {
    lua_pop(l_, 1);
  }

  operator Value();

  inline void push() {
    lua_pushvalue(l_, key_);
    lua_gettable(l_, -2);
    lua_remove(l_, -2);
  }

private:
  lua_State* l_;
  int key_;
  const NextType& next_;
};

// Represents a ParticleRotation residing in the Lua registry.
class Value : public BaseValue<Value> {
public:
  // Constructs a nil ParticleRotation.
  Value();

  // Constructs a ParticleRotation from the given reference.
  Value(const Reference& ref);

  // Constructs a ParticleRotation from a reference residing on the Lua stack.
  Value(lua_State* l, int stack_index);

  // Construts a new ParticleRotation with the given ParticleRotation and puts it in the registry.
  template<typename T>
  Value(lua_State* l, const T& ParticleRotation);

  // Push this ParticleRotation onto the Lua stack.
  void push() const;

  template<class T>
  IndexValue<Value> operator[](const T& key) const {
    return IndexValue<Value>(*this, ref_.l(), key);
  }

private:
  // A reference to the ParticleRotation we hold.
  Reference ref_;
};

template<typename T>
inline Value::Value(lua_State* l, const T& ParticleRotation) {

}

template<typename NextType>
IndexValue<NextType>::operator Value() {
  // Push our ParticleRotation onto the stack, construct a new Value with that pushed reference,
  // use PopStack to pop ourselves from the stack when done.
  impl::PopStack pop(l_, 1);
  push();
  return Value(l_, -1);
}

}

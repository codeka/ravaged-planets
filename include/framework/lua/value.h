#pragma once

#include <ostream>

#include <framework/lua/base.h>
#include <framework/lua/push.h>
#include <framework/lua/reference.h>

namespace fw::lua {

// Base class for any value-like object (values, index values, call return values, etc). 
template<typename Derived>
class BaseValue<Derived> {

private:
  inline Derived& derived() {
    return *static_cast<Derived*>(this);
  }

  inline const Derived& derived() const {
    return *static_cast<const Derived*>(this);
  }
};

// Allows us to log a Lua value.
template<class ValueType>
std::ostream& operator <<(std::ostream& os, const BaseValue<ValueType>& v) {
  // TODO: implement me
  return os;
}

class Value;

// This is the value returned from the indexing operation. We use this 'proxy'
// type so that we don't have to immediately move the return value of the []
// operator into the registry, it can stay as a temporary object in the stack.
template<typename NextType>
class IndexValue : public BaseValue<IndexValue<NextType>> {
  
  template<typename T>
  IndexValue(const NextType& next, lua_State* l, const T& key)
    : l_(l), key_(lua_gettop(l) + 1), next_(next) {
    push(l, key);
  }

  ~IndexValue();

  operator Value();

  void push();

private:
  lua_State* l_;
  int key_;
  const NextType& next_;
};

// Represents a value residing in the Lua registry.
class Value : public BaseValue<Value> {
  // Constructs a nil value.
  Value();

  // Constructs a value from the given reference.
  Value(const Reference& ref);

  // Constructs a value from a reference residing on the Lua stack.
  Value(lua_State* l, int stack_index);

  // Construts a new value with the given value and puts it in the registry.
  template<typename T>
  Value(lua_State* l, const T& value);

  // Push this value onto the Lua stack.
  void push() const;

  template<class T>
  IndexValue<Value> operator[](const T& key) const {
    return IndexValue<Value>(*this, ref_.l(), key);
  }

public:
  // A reference to the value we hold.
  Reference ref_;
};

template<typename T>
inline Value::Value(lua_State* l, const T& value) {

}

}

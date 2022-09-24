#pragma once

#include <ostream>

#include <framework/lua/base.h>
#include <framework/lua/push.h>
#include <framework/lua/reference.h>

namespace fw::lua {

// Base class for any value-like object (values, index values, call return values, etc). 
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
public:
  template<typename T>
  inline IndexValue(const NextType& next, lua_State* l, const T& key)
    : l_(l), key_index_(lua_gettop(l) + 1), next_(next) {
    lua::push(l, key);
  }

  inline ~IndexValue() {
    lua_pop(l_, 1);
  }

  operator Value();

  template<typename T>
  inline IndexValue& operator=(const T& value) {
    // Push the table (which is next_), then push the key (again), push the new value, call set table with the value
    // below the key. Pop the table again once we're done (lua_settable automatically pops the key & value).
    lua::push(l_, next_);
    lua_pushvalue(l_, key_index_);
    lua::push(l_, value);
    lua_settable(l_, -3);
    return *this;
  }

  template<class T>
  inline IndexValue<IndexValue> operator[](const T& key) {
    return IndexValue<IndexValue>(*this, ref_.l(), key);
  }

  inline void push() {
    lua_pushvalue(l_, key_);
    lua_gettable(l_, -2);
    lua_remove(l_, -2);
  }

  inline bool is_nil() {
    return !ref_;
  }

private:
  lua_State* l_;

  // Index on the stack of the key
  int key_index_;
  const NextType& next_;
};

// Represents a value residing in the Lua registry.
class Value : public BaseValue<Value> {
public:
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
  IndexValue<Value> operator[](const T& key) {
    return IndexValue<Value>(*this, ref_.l(), key);
  }

private:
  // A reference to the value we hold.
  Reference ref_;
};

template<typename T>
inline Value::Value(lua_State* l, const T& value) {

}

template<typename NextType>
IndexValue<NextType>::operator Value() {
  // Push our value onto the stack, construct a new Value with that pushed reference,
  // use PopStack to pop ourselves from the stack when done.
  impl::PopStack pop(l_, 1);
  push();
  return Value(l_, -1);
}

}

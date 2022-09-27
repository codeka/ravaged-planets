#pragma once

#include <iterator>
#include <ostream>
#include <boost/any.hpp>

#include <framework/lua/base.h>
#include <framework/lua/push.h>
#include <framework/lua/reference.h>

namespace fw::lua {

class Value;

class ValueIteratorEntry {
public:
  ValueIteratorEntry(lua_State* l) : l_(l) {
  }

  template<typename T>
  inline T value() const;
  template<>
  inline Value value() const;
  template<>
  inline std::string value() const;
  template<>
  inline float value() const;
  template<>
  inline int value() const;
  template<>
  inline boost::any value() const;

  // TODO: should we have our own enum here?
  int value_type() const {
    return lua_type(l_, -1);
  }

  template<typename T>
  inline T key() const;
  template<>
  inline Value key() const;
  template<>
  inline std::string key() const;


private:
  lua_State* l_;
};

// Iterate the keys and values of the given table.
class ValueIterator : public std::iterator<std::input_iterator_tag, ValueIteratorEntry> {
public:
  // Constructs an "end" ValueIterator that doesn't refer to anything.
  inline ValueIterator() : copies_(nullptr), l_(nullptr), entry_(nullptr), is_end_(true), num_to_pop_(0) {
  }

  inline ValueIterator(lua_State* l, Value& value);

  inline ValueIterator(const ValueIterator& copy)
      : copies_(copy.copies_), l_(copy.l_), entry_(copy.l_) {
    if (copies_ != nullptr) {
      (*copies_)++;
    }
  }

  inline ~ValueIterator() {
    if (copies_ != nullptr && num_to_pop_ > 0) {
      (*copies_)--;
      if ((*copies_) == 0) {
        // Pop the key & the table itself.
        lua_pop(l_, num_to_pop_);
      }
    }
  }

  friend void swap(ValueIterator& lhs, ValueIterator& rhs) {
    using std::swap;
    swap(lhs.l_, rhs.l_);
    swap(lhs.copies_, rhs.copies_);
    swap(lhs.entry_, rhs.entry_);
    swap(lhs.is_end_, rhs.is_end_);
    swap(lhs.num_to_pop_, rhs.num_to_pop_);
  }

  inline ValueIterator& operator=(ValueIterator& other) {
    ValueIterator tmp(other);
    swap(*this, other);
    return *this;
  }

  inline bool operator ==(const ValueIterator& other) {
    if (is_end_ && other.is_end_) {
      return true;
    }
    if (is_end_ || other.is_end_) {
      return false;
    }

    // TODO: same key?
    return true;
  }

  inline bool operator !=(const ValueIterator& other) {
    return !(*this == other);
  }

  inline ValueIterator& operator++() {
    // Remove the value, the key should still be there.
    lua_pop(l_, 1);
    if (lua_next(l_, -2) == 0) {
      is_end_ = true;

      // lua_next won't push anything if it returns 0, so only need to pop the table.
      num_to_pop_ = 1;
    }
    return *this;
  }

  inline const ValueIteratorEntry& operator*() const {
    return entry_;
  }

  inline const ValueIteratorEntry* operator->() const {
    return &entry_;
  }

private:
  // Pointer to a shared reference count. When all instances of the iterator are destucted, we need to pop the table
  // from the stack.
  int *copies_;
  lua_State* l_;
  ValueIteratorEntry entry_;
  bool is_end_;
  int num_to_pop_;
};

// Base class for any value-like object (values, index values, call return values, etc). 
template<typename Derived>
class BaseValue {
public:
  BaseValue(lua_State* l) : l_(l) {
  }

protected:
  lua_State* l_;

private:
  inline Derived& derived() {
    return *static_cast<Derived*>(this);
  }

  inline const Derived& derived() const {
    return *static_cast<const Derived*>(this);
  }
};

// This is the value returned from the indexing operation. We use this 'proxy' type so that we don't have to
// immediately move the return value of the [] operator into the registry, it can stay as a temporary object in the
// stack.
template<typename NextType>
class IndexValue : public BaseValue<IndexValue<NextType>> {
public:
  template<typename T>
  inline IndexValue(const NextType& next, lua_State* l, const T& key)
    : BaseValue<IndexValue<NextType>>(l), key_index_(lua_gettop(l) + 1), next_(next) {
    lua::push(l, key);
  }

  inline ~IndexValue() {
    lua_pop(l_, 1);
  }

  operator Value();
  operator std::string() const {
    push();
    impl::PopStack pop(l_, 1);
    size_t len = 0;
    const char* str = lua_tolstring(l_, -1, &len);
    return std::string(str, len);
  }

  operator float() const {
    push();
    impl::PopStack pop(l_, 1);
    return static_cast<float>(lua_tonumber(l_, -1));
  }

  operator int() const {
    push();
    impl::PopStack pop(l_, 1);
    return static_cast<int>(lua_tonumber(l_, -1));
  }

  operator bool() const {
    push();
    impl::PopStack pop(l_, 1);
    return lua_toboolean(l_, -1);
  }

  template<typename T>
  T value() {
    push();
    impl::PopStack pop(l_, 1);

    return peek<T>(l_, -1);
  }

  template<typename T>
  inline IndexValue& operator=(const T& value) {
    // Push the table (which is next_), then push the key (again), push the new value, call set table with the value
    // below the key. Pop the table again once we're done (lua_settable automatically pops the key & value).
    lua::push(l_, next_);
    impl::PopStack pop(l_, 1);

    lua_pushvalue(l_, key_index_);
    lua::push(l_, value);
    lua_settable(l_, -3);
    return *this;
  }

  template<class T>
  inline IndexValue<IndexValue> operator[](const T& key) {
    return IndexValue<IndexValue>(*this, l_, key);
  }

  inline void push() const {
    // push the table, push the key, get the value (pops the key and pushes the value)
    lua::push(l_, next_);
    lua_pushvalue(l_, key_index_);
    lua_gettable(l_, -2);

    // pop the table (one below the value)
    lua_remove(l_, -2);
  }

  inline bool is_nil() {
    return !ref_;
  }

  std::string debug_string() const {
    push();
    impl::PopStack pop(l_, 1);

    // TODO: check if it's a table and convert that?
    {
      size_t len = 0;
      const char* str = luaL_tolstring(l_, -1, &len);
      impl::PopStack pop2(l_, 1);
      return std::string(str, len);
    }
  }

private:
  // Index on the stack of the key
  int key_index_;
  const NextType& next_;
};

// Represents a value residing in the Lua registry.
class Value : public BaseValue<Value> {
public:
  // Constructs a value from a reference residing on the Lua stack.
  Value(lua_State* l, int stack_index)
    : BaseValue<Value>(l), ref_(l, stack_index) {
  }

  // Construts a new value with the given value and puts it in the registry.
  template<typename T>
  Value(lua_State* l, const T& value)
    : BaseValue<Value>(l) {
    push(value);
    ref_(l, -1);
  }

  // Push this value onto the Lua stack.
  void push() const {
    ref_.push();
  }

  bool is_nil() const {
    return !ref_;
  }

  template<class T>
  bool has_key(const T& key) {
    impl::PopStack pop(l_, 2);
    push();
    lua::push(l_, key);
    return lua_gettable(l_, -2) != LUA_TNIL;
  }

  template<class T>
  IndexValue<Value> operator[](const T& key) {
    return IndexValue<Value>(*this, ref_.l(), key);
  }

  ValueIterator begin() {
    return ValueIterator(l_, *this);
  }

  ValueIterator end() {
    return ValueIterator();
  }

  std::string debug_string() const {
    push();
    impl::PopStack pop(l_, 1);

    int is_num = 0;
    lua_Number n = lua_tonumberx(l_, -1, &is_num);
    if (is_num != 0) {
      return std::to_string(n);
    }

    // TODO: check if it's a table and convert that.
    {
      size_t len = 0;
      const char* str = luaL_tolstring(l_, -1, &len);
      impl::PopStack pop2(l_, 1);
      return std::string(str, len);
    }
  }

private:
  // A reference to the value we hold.
  Reference ref_;
};

// Allows us to log a Lua value.
inline std::ostream& operator <<(std::ostream& os, const Value& v) {
  os << v.debug_string();
  return os;
}

template<>
Value ValueIteratorEntry::key() const {
  return Value(l_, -2);
}

template<>
std::string ValueIteratorEntry::key() const {
  size_t len = 0;
  const char* str = lua_tolstring(l_, -2, &len);
  return std::string(str, len);
}

template<>
Value ValueIteratorEntry::value() const {
  return Value(l_, -1);
}

template<>
std::string ValueIteratorEntry::value() const {
  size_t len = 0;
  const char* str = lua_tolstring(l_, -1, &len);
  return std::string(str, len);
}

template<>
float ValueIteratorEntry::value() const {
  return static_cast<float>(lua_tonumber(l_, -1));
}

template<>
int ValueIteratorEntry::value() const {
  return static_cast<int>(lua_tonumber(l_, -1));
}

template<>
boost::any ValueIteratorEntry::value() const {
  switch (lua_type(l_, -1)) {
  case LUA_TNUMBER:
    return value<float>();

  case LUA_TSTRING:
    return value<std::string>();

  case LUA_TBOOLEAN:
    return static_cast<bool>(lua_toboolean(l_, -1));

  case LUA_TTABLE:
    return value<Value>();

  default:
    // TODO: support for LUA_TFUNCTION, LUA_TUSERDATA, LUA_TTHREAD and LUA_TLIGHTUSERDATA?
    return boost::any();
  }
}

ValueIterator::ValueIterator(lua_State* l, Value& value) : copies_(new int), l_(l), entry_(l), is_end_(false) {
  value.push();
  lua_pushnil(l_);

  if (lua_next(l_, -2) == 0) {
    lua_pop(l_, 2);

    // If there's no keys at all, just become an end() iterator.
    is_end_ = true;
    l_ = nullptr;
    delete copies_;
    copies_ = nullptr;
  } else {
    (*copies_) = 1;
    num_to_pop_ = 2;
  }
}

template<typename NextType>
IndexValue<NextType>::operator Value() {
  // Push our value onto the stack, construct a new Value with that pushed reference,
  // use PopStack to pop ourselves from the stack when done.
  push();
  impl::PopStack pop(l_, 1);
  return Value(l_, -1);
}

}

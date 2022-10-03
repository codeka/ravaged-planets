#pragma once

#include <string>
#include <map>
#include <optional>

#include <framework/lua/method.h>
#include <framework/lua/value.h>

namespace fw::lua {

// Represents a piece of userdata that backs a C++ class of type T. You can add methods and whatnot to the Userdata
// type and they will be added to the metatable of the underlying userdata in Lua. In this way, you can "link" Lua to
// C++.
template<typename T>
class Userdata : public BaseValue<Userdata<T>> {
public:
  // Constructs a new Userdata with a nil value.
  Userdata() : BaseValue<Userdata<T>>(nullptr) {
  }

  // Constructs a new Userdata that refers to the given object. When calling this constructor, you must use in-place
  // new with the userdata on the stack, like so:
  // new(lua_newuserdata(l_, sizeof(Userdata<T>))) Userdata<T>(l_, object)
  Userdata(lua_State* l, T* owner) : BaseValue<Userdata<T>>(l), ref_(l_, -1), owner_(owner) {
  }

  // Attempt to cast the given value as a Userdata. Returns nullopt if it's not valid.
  static inline std::optional<Userdata<T>> from(const Value &value) {
    value.push();
    impl::PopStack pop(value.l(), 1);

    void* p = luaL_testudata(value.l(), -1, T::metatable.name().c_str());
    if (p == nullptr) {
      return std::nullopt;
    }

    return *(reinterpret_cast<Userdata<T>*>(p));
  }

  inline const T* owner() const {
    return owner_;
  }
  inline T* owner() {
    return owner_;
  }

  inline bool is_nil() const {
    return ref_.is_nil();
  }

  inline void push() const {
    ref_.push();
  }

private:
  Reference ref_;
  T* owner_;
};

}  // namespace fw::lua

#pragma once

#include <framework/lua/push.h>
#include <framework/lua/userdata.h>

namespace fw::lua {

template<typename Owner>
class MethodContext {
public:
  MethodContext(lua_State* l, Userdata<Owner>* userdata) : l_(l), userdata_(userdata), num_return_values_(0) {
  }

  // Gets the "owner" of this method call.
  Owner* owner() {
    return userdata_->owner();
  }

  int num_return_values() const {
    return num_return_values_;
  }

  template<typename T>
  T arg(int index) const {
    // We add 2 from the index. 1 because Lua indices start at 1 and 2 because the first element on the stack will be
    // the "this" pointer.
    return peek<T>(l_, index + 2);
  }

  template<typename T>
  void return_value(const T& value) {
    push(l_, value);
    num_return_values_++;
  }

  // Call this if you push a value onto the stack that you want to return.
  void record_return_value() {
    num_return_values_++;
  }

private:
  lua_State* l_;
  Userdata<Owner>* userdata_;
  int num_return_values_;
};

template<typename Owner>
class PropertyContext {
public:
  PropertyContext(lua_State* l, Owner* owner) : l_(l), owner_(owner), has_return_value_(false) {
  }

  // Gets the "owner" of this method call.
  Owner* owner() {
    return owner_;
  }

  template<typename T>
  inline void return_value(const T& value) {
    if (has_return_value_) {
      // TODO: this is not valid.
      return;
    }

    push(l_, value);
    has_return_value_ = true;
  }

  inline bool has_return_value() const {
    return has_return_value_;
  }

private:
  bool has_return_value_;
  lua_State* l_;
  Owner* owner_;
};

template<typename Owner>
using MethodCall = std::function<void(MethodContext<Owner>&)>;

template<typename Owner>
using PropertyCall = std::function<void(PropertyContext<Owner>&)>;

// Contains the information we care about for a method call. Basically that means the std::function we *actually* call
// in response to a call from Lua.
template<typename Owner>
class MethodClosure {
public:
  MethodClosure(MethodCall<Owner> method) : method_(method) {
  }

  int call(lua_State* l, Userdata<Owner>* userdata) const {
    MethodContext<Owner> ctx(l, userdata);
    method_(ctx);
    return ctx.num_return_values();
  }

private:
  MethodCall<Owner> method_;
};

// Pushes a method that is associated with Owner::metatable.
template<typename Owner>
inline void push(lua_State* l, MethodCall<Owner> method) {
  MethodClosure<Owner>* closure =
    new(lua_newuserdata(l, sizeof(MethodClosure<Owner>))) MethodClosure<Owner>(method);

  lua_pushcclosure(l, Owner::metatable.callback(), 1);
}

}  // namespace fw::lua

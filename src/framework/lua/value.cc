#include <framework/lua/value.h>

namespace fw::lua {

IndexValue::~IndexValue() {
  lua_pop(l_, 1);
}

IndexValue::operator Value() {
  // Push our value onto the stack, construct a new Value with that pushed reference,
  // use PopStack to pop ourselves from the stack when done.
  impl::PopStack pop(l_, 1);
  push();
  return Value(l_, -1);
}

void IndexValue::push() {
  lua_pushvalue(l_, key_);
  lua_gettable(l_, -2);
  lua_remove(l_, -2);
}

Value::Value() {
}

Value::Value(const Reference& ref)
 : ref_(ref) {

}

Value::Value(lua_State* l, int stack_index)
 : ref_(l, stack_index) {
}

void Value::push() const {
  ref_.push();
}

}
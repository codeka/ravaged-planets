#include <framework/lua/value.h>

namespace fw::lua {

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
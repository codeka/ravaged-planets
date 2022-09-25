
//#include <luabind/luabind.hpp>
//#include <luabind/adopt_policy.hpp>

#include <framework/lua.h>

#include <game/ai/unit_wrapper.h>
#include <game/simulation/player.h>
#include <game/simulation/orders.h>
#include <game/entities/entity.h>
#include <game/entities/ownable_component.h>
#include <game/entities/orderable_component.h>

namespace game {
// registers our class and methods and stuff in the given lua_State
void unit_wrapper::register_class(lua_State *state) {
//  luabind::module(state) [
//      luabind::class_<unit_wrapper>("Unit")
//          .def(luabind::constructor<>())
//          .property("kind", &unit_wrapper::l_get_kind)
//          .property("player_no", &unit_wrapper::l_get_player_no)
//          .property("state", &unit_wrapper::l_get_state)
//  ];
}

void unit_wrapper::set_entity(std::weak_ptr<ent::Entity> const &ent) {
  entity_ = ent;

  std::shared_ptr<ent::Entity> sp = entity_.lock();
  if (sp) {
    _ownable = sp->get_component<ent::OwnableComponent>();
    _orderable = sp->get_component<ent::OrderableComponent>();
  }
}

std::string unit_wrapper::l_get_kind() {
  std::shared_ptr<ent::Entity> ent = entity_.lock();
  if (ent) {
    return ent->get_name();
  } else {
    return "";
  }
}

std::string unit_wrapper::l_get_state() {
  std::shared_ptr<ent::Entity> Entity = entity_.lock();
  if (Entity && _orderable != nullptr) {
    std::shared_ptr<game::order> curr_order = _orderable->get_current_order();
    if (curr_order) {
      return curr_order->get_state_name();
    }
  }

  return "idle";
}

int unit_wrapper::l_get_player_no() {
  std::shared_ptr<ent::Entity> Entity = entity_.lock();
  if (Entity) {
    return static_cast<int>(_ownable->get_owner()->get_player_no());
  }

  return 0;
}

}

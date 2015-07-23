#include <framework/lua.h>

#include <game/ai/unit_wrapper.h>
#include <game/simulation/player.h>
#include <game/simulation/orders.h>
#include <game/entities/entity.h>
#include <game/entities/ownable_component.h>
#include <game/entities/orderable_component.h>

namespace game {

char const unit_wrapper::class_name[] = "Unit";
fw::lua_registrar<unit_wrapper>::method_definition unit_wrapper::methods[] = {
   {"kind", &unit_wrapper::l_get_kind},
   {"player_no", &unit_wrapper::l_get_player_no},
   {"state", &unit_wrapper::l_get_state},
   {nullptr, nullptr}
};

void unit_wrapper::set_entity(std::weak_ptr<ent::entity> const &ent) {
  _entity = ent;

  std::shared_ptr<ent::entity> sp = _entity.lock();
  if (sp) {
    _ownable = sp->get_component<ent::ownable_component>();
    _orderable = sp->get_component<ent::orderable_component>();
  }
}

int unit_wrapper::l_get_kind(fw::lua_context &ctx) {
  std::string kind = get_kind();
  lua_pushstring(ctx.get_state(), kind.c_str());
  return 1;
}

int unit_wrapper::l_get_player_no(fw::lua_context &ctx) {
  int player_no = get_player_no();
  lua_pushvalue(ctx.get_state(), player_no);
  return 1;
}

int unit_wrapper::l_get_state(fw::lua_context &ctx) {
  std::string kind = get_state();
  lua_pushstring(ctx.get_state(), kind.c_str());
  return 1;
}

std::string unit_wrapper::get_kind() {
  std::shared_ptr<ent::entity> ent = _entity.lock();
  if (ent)
    return ent->get_name();
  else
    return "";
}

std::string unit_wrapper::get_state() {
  std::shared_ptr<ent::entity> entity = _entity.lock();
  if (entity && _orderable != 0) {
    std::shared_ptr<game::order> curr_order = _orderable->get_current_order();
    if (curr_order) {
      return curr_order->get_state_name();
    }
  }

  return "idle";
}

int unit_wrapper::get_player_no() {
  std::shared_ptr<ent::entity> entity = _entity.lock();
  if (entity) {
    return static_cast<int>(_ownable->get_owner()->get_player_no());
  }

  return 0;
}

}

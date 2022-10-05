
#include <framework/lua.h>
#include <framework/logging.h>

#include <game/ai/unit_wrapper.h>
#include <game/simulation/player.h>
#include <game/simulation/orders.h>
#include <game/entities/entity.h>
#include <game/entities/ownable_component.h>
#include <game/entities/orderable_component.h>

namespace game {

LUA_DEFINE_METATABLE(UnitWrapper)
    .property("kind", UnitWrapper::l_get_kind)
    .property("state", UnitWrapper::l_get_state)
    .property("player_no", UnitWrapper::l_get_player_no);

UnitWrapper::UnitWrapper(std::weak_ptr<ent::Entity> ent, std::weak_ptr<fw::lua::LuaContext> ctx, fw::lua::Value script)
    : entity_(ent), ctx_(ctx), script_(script) {
  std::shared_ptr<ent::Entity> sp = entity_.lock();
  if (sp) {
    ownable_ = sp->get_component<ent::OwnableComponent>();
    orderable_ = sp->get_component<ent::OrderableComponent>();
  } else {
    ownable_ = nullptr;
    orderable_ = nullptr;
  }

  if (!script_.is_nil()) {
    auto ctx = ctx_.lock();
    if (ctx) {
      script_["__init"](ctx->wrap(this));
    }
  }
}

/* static */
void UnitWrapper::l_get_kind(fw::lua::PropertyContext<UnitWrapper>& ctx) {
  ctx.return_value(ctx.owner()->get_kind());
}

std::string UnitWrapper::get_kind() {
  std::shared_ptr<ent::Entity> ent = entity_.lock();
  if (ent) {
    return ent->get_name();
  } else {
    return "";
  }
}

/* static */
void UnitWrapper::l_get_state(fw::lua::PropertyContext<UnitWrapper>& ctx) {
  ctx.return_value(ctx.owner()->get_state());
}

std::string UnitWrapper::get_state() {
  std::shared_ptr<ent::Entity> entity = entity_.lock();
  if (entity && orderable_ != nullptr) {
    std::shared_ptr<game::Order> curr_order = orderable_->get_current_order();
    if (curr_order) {
      return curr_order->get_state_name();
    }
  }

  return "idle";
}

/* static */
void UnitWrapper::l_get_player_no(fw::lua::PropertyContext<UnitWrapper>& ctx) {
  ctx.return_value(ctx.owner()->get_player_no());
}

int UnitWrapper::get_player_no() {
  std::shared_ptr<ent::Entity> entity = entity_.lock();
  if (entity) {
    return static_cast<int>(ownable_->get_owner()->get_player_no());
  }

  return 0;
}

}

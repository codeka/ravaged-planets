#pragma once

#include <memory>

#include <framework/lua.h>

namespace ent {
class Entity;
class OwnableComponent;
class OrderableComponent;
}

namespace game {

// Wraps entities so that Lua classes can inherit from and call methods on and so on.
class UnitWrapper {
private:
  std::weak_ptr<ent::Entity> entity_;
  ent::OwnableComponent *ownable_;
  ent::OrderableComponent *orderable_;

  static void l_get_kind(fw::lua::PropertyContext<UnitWrapper>& ctx);
  std::string get_kind();
  static void l_get_player_no(fw::lua::PropertyContext<UnitWrapper>& ctx);
  int get_player_no();
  static void l_get_state(fw::lua::PropertyContext<UnitWrapper>& ctx);
  std::string get_state();

public:
  UnitWrapper(std::weak_ptr<ent::Entity> entity);

  // registers the unit_wrapper as the "Unit" class in Lua.
  static void register_class(lua_State *state);

  std::weak_ptr<ent::Entity> get_entity() const {
    return entity_;
  }

  LUA_DECLARE_METATABLE(UnitWrapper);
};

}

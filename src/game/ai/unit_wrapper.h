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

  std::string l_get_kind();
  int l_get_player_no();
  std::string l_get_state();

public:
  // registers the unit_wrapper as the "Unit" class in Lua.
  static void register_class(lua_State *state);

  // this must be called before you do anything with this wrapper
  void set_entity(std::weak_ptr<ent::Entity> const &ent);
  std::weak_ptr<ent::Entity> get_entity() const {
    return entity_;
  }
};

}

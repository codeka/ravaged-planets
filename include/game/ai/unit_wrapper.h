#pragma once

#include <memory>

struct lua_State;

namespace ent {
class entity;
class ownable_component;
class orderable_component;
}

namespace game {

/**
 * Wraps entities so that Lua classes can inherit from and call methods on and so on.
 */
class unit_wrapper: public luabind::wrap_base {
private:
  std::weak_ptr<ent::entity> _entity;
  ent::ownable_component *_ownable;
  ent::orderable_component *_orderable;

  std::string l_get_kind();
  int l_get_player_no();
  std::string l_get_state();

public:
  // registers the unit_wrapper as the "Unit" class in LUA.
  static void register_class(lua_State *state);

  // this must be called before you do anything with this wrapper
  void set_entity(std::weak_ptr<ent::entity> const &ent);
  std::weak_ptr<ent::entity> get_entity() const {
    return _entity;
  }
};

}

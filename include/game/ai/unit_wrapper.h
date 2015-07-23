#pragma once

#include <memory>

#include <framework/lua.h>

namespace ent {
class entity;
class ownable_component;
class orderable_component;
}

namespace game {

/**
 * Wraps entities so that Lua classes can inherit from and call methods on and so on.
 */
class unit_wrapper {
private:
  std::weak_ptr<ent::entity> _entity;
  ent::ownable_component *_ownable;
  ent::orderable_component *_orderable;

  int l_get_kind(fw::lua_context &ctx);
  int l_get_player_no(fw::lua_context &ctx);
  int l_get_state(fw::lua_context &ctx);

  std::string get_kind();
  int get_player_no();
  std::string get_state();

public:
  static char const class_name[];
  static fw::lua_registrar<unit_wrapper>::method_definition methods[];

  // this must be called before you do anything with this wrapper
  void set_entity(std::weak_ptr<ent::entity> const &ent);
  std::weak_ptr<ent::entity> get_entity() const {
    return _entity;
  }
};

}

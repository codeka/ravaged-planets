#include <game/entities/entity_factory.h>
#include <game/entities/ownable_component.h>

//#include <game/simulation/simulation_thread.h>
//#include <game/simulation/local_player.h>
//#include <game/ai/ai_player.h>

namespace ent {

ENT_COMPONENT_REGISTER("ownable", ownable_component);

ownable_component::ownable_component() :
    _owner(nullptr) {
}

ownable_component::~ownable_component() {
}

void ownable_component::set_owner(game::player *owner) {
  _owner = owner;
  owner_changed_event(this);
}

bool ownable_component::is_local_player() const {
//  if (_owner == nullptr)
    return false;

//  game::player *local_player = game::simulation_thread::get_instance()->get_local_player();
//  return (local_player == _owner);
}

bool ownable_component::is_local_or_ai_player() const {
//  if (_owner == nullptr)
    return false;

//  if (is_local_player())
 //   return true;

//  game::ai_player *ai_owner = dynamic_cast<game::ai_player *>(_owner);
  //if (ai_owner != nullptr)
    //return true;

  //return false;
}

}

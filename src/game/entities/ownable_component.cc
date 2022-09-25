#include <game/entities/entity_factory.h>
#include <game/entities/ownable_component.h>

#include <game/simulation/simulation_thread.h>
#include <game/simulation/local_player.h>
#include <game/ai/ai_player.h>

namespace ent {

ENT_COMPONENT_REGISTER("Ownable", OwnableComponent);

OwnableComponent::OwnableComponent() :
    owner_(nullptr) {
}

OwnableComponent::~OwnableComponent() {
}

void OwnableComponent::set_owner(game::Player *owner) {
  owner_ = owner;
  owner_changed_event(this);
}

bool OwnableComponent::is_local_player() const {
  if (owner_ == nullptr)
    return false;

  game::Player *local_player = game::SimulationThread::get_instance()->get_local_player();
  return (local_player == owner_);
}

bool OwnableComponent::is_local_or_ai_player() const {
  if (owner_ == nullptr)
    return false;

  if (is_local_player())
    return true;

  game::AIPlayer *ai_owner = dynamic_cast<game::AIPlayer *>(owner_);
  if (ai_owner != nullptr)
    return true;

  return false;
}

}

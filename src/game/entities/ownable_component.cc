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

void OwnableComponent::set_owner(std::shared_ptr<game::Player> const &owner) {
  owner_ = owner;
  owner_changed_event(this);
}

bool OwnableComponent::is_local_player() const {
  if (owner_ == nullptr)
    return false;

  auto local_player = game::SimulationThread::get_instance()->get_local_player();
  return (local_player == owner_);
}

bool OwnableComponent::is_local_or_ai_player() const {
  if (owner_ == nullptr)
    return false;

  if (is_local_player())
    return true;

  auto ai_owner = std::dynamic_pointer_cast<std::shared_ptr<game::AIPlayer>>(owner_);
  if (ai_owner != nullptr) {
    return true;
  }

  return false;
}

}

#include <game/entities/entity_factory.h>
#include <game/entities/orderable_component.h>
#include <game/simulation/commands.h>
#include <game/simulation/orders.h>
#include <game/simulation/simulation_thread.h>

namespace ent {

// register the orderable component with the entity_factory
ENT_COMPONENT_REGISTER("Orderable", OrderableComponent);

OrderableComponent::OrderableComponent() :
    order_pending_(false) {
}

OrderableComponent::~OrderableComponent() {
}

void OrderableComponent::execute_order(std::shared_ptr<game::Order> const &order) {
  curr_order_ = order;
  order_pending_ = false;
  curr_order_->begin(entity_);
}

void OrderableComponent::update(float dt) {
  if (curr_order_) {
    // if we've currently got an order, update it and check whether it's finished
    curr_order_->update(dt);
    if (curr_order_->is_complete()) {
      curr_order_.reset();
    }
  }

  if (!curr_order_ && !order_pending_) {
    // if we don't have an order, check whether any has been queued since the last
    // update and execute a command to start it off
    if (orders_.size() > 0) {
      std::shared_ptr<game::Order> next_order = orders_.front();
      orders_.pop();

      std::shared_ptr<game::OrderCommand> cmd(game::create_command<game::OrderCommand>());
      cmd->order = next_order;
      cmd->Entity = std::shared_ptr<ent::Entity>(entity_)->get_id();
      game::SimulationThread::get_instance()->post_command(cmd);

      // mark that we've got an order pending, it'll take a few frames
      // for it to actually come back to us
      order_pending_ = true;
    }
  }
}

void OrderableComponent::issue_order(std::shared_ptr<game::Order> const &order) {
  orders_.push(order);
}

int OrderableComponent::get_order_count() const {
  return orders_.size() + (curr_order_ ? 1 : 0);
}

std::shared_ptr<game::Order> OrderableComponent::get_current_order() const {
  return curr_order_;
}

}

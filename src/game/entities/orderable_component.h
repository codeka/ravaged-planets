//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#pragma once

#include <map>
#include <memory>
#include <queue>

#include <game/entities/entity.h>

namespace game {
class order;
}

namespace ent {

// The orderable component is attached to each entity that can execute orders
// from a local (or AI) player.
class orderable_component: public entity_component {
private:
  std::shared_ptr<game::order> _curr_order;
  bool _order_pending;
  std::queue<std::shared_ptr<game::order>> _orders;

public:
  static const int identifier = 550;
  virtual int get_identifier() {
    return identifier;
  }

  orderable_component();
  ~orderable_component();

  virtual void update(float dt);

  // begins actually executing an order. This should only be called by the order_command
  // when the simulation thread has deemed it's time to begin execution.
  void execute_order(std::shared_ptr<game::order> const &order);

  // adds an order to the queue. When it gets to the head of the queue, it will be sent to
  // the simulation thread for actual processing. When the simulation thread comes back, it'll
  // call execute_order to actually begin the execution.
  void issue_order(std::shared_ptr<game::order> const &order);

  // gets the total number of currently executing + waiting orders.
  int get_order_count() const;
  std::shared_ptr<game::order> get_current_order() const;
};

}

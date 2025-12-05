#include <functional>

#include <framework/cursor.h>
#include <framework/input.h>
#include <framework/framework.h>

#include <game/entities/entity.h>
#include <game/entities/entity_manager.h>
#include <game/entities/weapon_component.h>
#include <game/entities/ownable_component.h>
#include <game/entities/selectable_component.h>
#include <game/simulation/orders.h>
#include <game/simulation/simulation_thread.h>
#include <game/simulation/local_player.h>
#include <game/world/cursor_handler.h>
#include <game/world/terrain.h>
#include <game/world/world.h>

namespace game {

using namespace std::placeholders;

CursorHandler::CursorHandler() :
    entities_(nullptr), terrain_(nullptr) {
}

CursorHandler::~CursorHandler() {
}

void CursorHandler::initialize() {
  fw::Input *Input = fw::Framework::get_instance()->get_input();
  keybind_tokens_.push_back(Input->bind_function("select", std::bind(&CursorHandler::on_key_select, this, _1, _2)));
  keybind_tokens_.push_back(
      Input->bind_function("deselect", std::bind(&CursorHandler::on_key_deselect, this, _1, _2)));

  entities_ = game::World::get_instance()->get_entity_manager();
  terrain_ = game::World::get_instance()->get_terrain();
}

void CursorHandler::update() {
  bool highlight = false;
  fw::Color highlight_color;
  std::string cursor_name = "arrow";

  if (entities_ == nullptr) {
    return;
  }

  entity_under_cursor_ = entities_->get_entity_at_cursor();
  std::shared_ptr<ent::Entity> entity_under_cursor = entity_under_cursor_.lock();
  if (entity_under_cursor) {
    auto local_player = game::SimulationThread::get_instance()->get_local_player();

    ent::OwnableComponent *ownable = entity_under_cursor->get_component<ent::OwnableComponent>();
    if (ownable != nullptr) {
      if (ownable->get_owner() == local_player) {
        //highlight = true;
        //highlight_color = fw::color(1, 1, 1);
        cursor_name = "select";
      } else if (entities_->get_selection().size() > 0) {
        // if it's not ours and we have a selection, highlight with
        // red to indicate we can attack
        highlight = true;
        highlight_color = fw::Color(1, 0, 0);
        cursor_name = "attack";
      }
    } else {
      // should never happen.
      cursor_name = "i-beam";
    }
  } else {
    // if we don't have an Entity under our cursor, but we *do* have one selected, then we want
    // the "move" cursor!
    if (entities_->get_selection().size() > 0) {
      cursor_name = "move";
    }
  }

  fw::Framework::get_instance()->get_cursor()->set_cursor(1, cursor_name);

  std::shared_ptr<ent::Entity> last_highlighted = last_highlighted_.lock();
  if (last_highlighted) {
    ent::SelectableComponent *lh_selectable = last_highlighted->get_component<ent::SelectableComponent>();
    lh_selectable->unhighlight();
  }

  if (highlight) {
    ent::SelectableComponent *selectable = entity_under_cursor->get_component<ent::SelectableComponent>();
    selectable->highlight(highlight_color);
    last_highlighted_ = entity_under_cursor_;
  }
}

void CursorHandler::destroy() {
  fw::Input *input = fw::Framework::get_instance()->get_input();
  for (int token : keybind_tokens_) {
    input->unbind_key(token);
  }
  keybind_tokens_.clear();
}

void CursorHandler::on_key_select(std::string, bool is_down) {
  std::shared_ptr<ent::Entity> entity_under_cursor = entity_under_cursor_.lock();
  if (!is_down) {
    if (entity_under_cursor) {
      // todo: if it's ours, add it to our selection otherwise, attack!

      if (entities_->get_selection().size() == 0) {
        // if we don't have anything selected yet, select this one...
        entities_->add_selection(entity_under_cursor);
      } else {
        // if we've got entities selected, order them to attack this guy!
        for (auto it = entities_->get_selection().begin(); it != entities_->get_selection().end(); ++it) {
          std::shared_ptr<ent::Entity> ent = (*it).lock();
          if (!ent)
            continue;

          ent::OrderableComponent *orderable = ent->get_component<ent::OrderableComponent>();
          if (orderable != nullptr) {
            std::shared_ptr<AttackOrder> order(create_order<AttackOrder>());
            order->target = entity_under_cursor->get_id();
            orderable->issue_order(order);
          }
        }
      }
    } else {
      std::shared_ptr<MoveOrder> order(create_order<MoveOrder>());
      order->goal = terrain_->get_cursor_location();
//      LOG(DBG) << "cursor_location: " << order->goal;
      for (auto it = entities_->get_selection().begin(); it != entities_->get_selection().end(); ++it) {
        std::shared_ptr<ent::Entity> ent = it->lock();
        if (!ent)
          continue;

        ent::OrderableComponent *orderable = ent->get_component<ent::OrderableComponent>();
        if (orderable != nullptr) {
          orderable->issue_order(order);
        }
      }
    }
  }
}

void CursorHandler::on_key_deselect(std::string, bool is_down) {
  if (!is_down) {
    entities_->clear_selection();
  }
}

}

#include <functional>
#include <boost/foreach.hpp>

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

cursor_handler::cursor_handler() :
    _entities(nullptr), _terrain(nullptr) {
}

cursor_handler::~cursor_handler() {
}

void cursor_handler::initialize() {
  fw::input *input = fw::framework::get_instance()->get_input();
  _keybind_tokens.push_back(input->bind_function("select", std::bind(&cursor_handler::on_key_select, this, _1, _2)));
  _keybind_tokens.push_back(
      input->bind_function("deselect", std::bind(&cursor_handler::on_key_deselect, this, _1, _2)));

  _entities = game::world::get_instance()->get_entity_manager();
  _terrain = game::world::get_instance()->get_terrain();
}

void cursor_handler::update() {
  bool highlight = false;
  fw::colour highlight_colour;
  std::string cursor_name = "arrow";

  if (_entities == nullptr) {
    return;
  }

  _entity_under_cursor = _entities->get_entity_at_cursor();
  std::shared_ptr<ent::entity> entity_under_cursor = _entity_under_cursor.lock();
  if (entity_under_cursor) {
    local_player *lplyr = game::simulation_thread::get_instance()->get_local_player();

    ent::ownable_component *ownable = entity_under_cursor->get_component<ent::ownable_component>();
    if (ownable != nullptr) {
      if (ownable->get_owner() == lplyr) {
        //highlight = true;
        //highlight_colour = fw::colour(1, 1, 1);
        cursor_name = "select";
      } else if (_entities->get_selection().size() > 0) {
        // if it's not ours and we have a selection, highlight with
        // red to indicate we can attack
        highlight = true;
        highlight_colour = fw::colour(1, 0, 0);
        cursor_name = "attack";
      }
    } else {
      // should never happen.
      cursor_name = "i-beam";
    }
  } else {
    // if we don't have an entity under our cursor, but we *do* have one selected, then we want
    // the "move" cursor!
    if (_entities->get_selection().size() > 0) {
      cursor_name = "move";
    }
  }

  fw::framework::get_instance()->get_cursor()->set_cursor(1, cursor_name);

  std::shared_ptr<ent::entity> last_highlighted = _last_highlighted.lock();
  if (last_highlighted) {
    ent::selectable_component *lh_selectable = last_highlighted->get_component<ent::selectable_component>();
    lh_selectable->unhighlight();
  }

  if (highlight) {
    ent::selectable_component *selectable = entity_under_cursor->get_component<ent::selectable_component>();
    selectable->highlight(highlight_colour);
    _last_highlighted = _entity_under_cursor;
  }
}

void cursor_handler::destroy() {
  fw::input *input = fw::framework::get_instance()->get_input();
  BOOST_FOREACH(int token, _keybind_tokens) {
    input->unbind_key(token);
  }
  _keybind_tokens.clear();
}

void cursor_handler::on_key_select(std::string, bool is_down) {
  std::shared_ptr<ent::entity> entity_under_cursor = _entity_under_cursor.lock();
  if (!is_down) {
    if (entity_under_cursor) {
      // todo: if it's ours, add it to our selection otherwise, attack!

      if (_entities->get_selection().size() == 0) {
        // if we don't have anything selected yet, select this one...
        _entities->add_selection(entity_under_cursor);
      } else {
        // if we've got entities selected, order them to attack this guy!
        for (auto it = _entities->get_selection().begin(); it != _entities->get_selection().end(); ++it) {
          std::shared_ptr<ent::entity> ent = (*it).lock();
          if (!ent)
            continue;

          ent::orderable_component *orderable = ent->get_component<ent::orderable_component>();
          if (orderable != nullptr) {
            std::shared_ptr<attack_order> order(create_order<attack_order>());
            order->target = entity_under_cursor->get_id();
            orderable->issue_order(order);
          }
        }
      }
    } else {
      std::shared_ptr<move_order> order(create_order<move_order>());
      order->goal = _terrain->get_cursor_location();
      for (auto it = _entities->get_selection().begin(); it != _entities->get_selection().end(); ++it) {
        std::shared_ptr<ent::entity> ent = it->lock();
        if (!ent)
          continue;

        ent::orderable_component *orderable = ent->get_component<ent::orderable_component>();
        if (orderable != nullptr) {
          orderable->issue_order(order);
        }
      }
    }
  }
}

void cursor_handler::on_key_deselect(std::string, bool is_down) {
  if (!is_down) {
    _entities->clear_selection();
  }
}

}

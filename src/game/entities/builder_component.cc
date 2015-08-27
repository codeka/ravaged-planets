#include <functional>
#include <boost/foreach.hpp>

#include <game/entities/entity_factory.h>
#include <game/entities/builder_component.h>
#include <game/entities/selectable_component.h>
#include <game/entities/position_component.h>
#include <game/entities/ownable_component.h>
#include <game/entities/particle_effect_component.h>
#include <game/screens/hud/build_window.h>
//#include <game/screens/hud/chat_window.h>
#include <game/simulation/simulation_thread.h>
#include <game/simulation/commands.h>

namespace ent {

using namespace std::placeholders;

// register the builder component with the entity_factory
ENT_COMPONENT_REGISTER("Builder", builder_component);

builder_component::builder_component() :
    _time_to_build(0.0f) {
}

builder_component::~builder_component() {
}

void builder_component::initialize() {
  std::shared_ptr<entity> entity(_entity);
  selectable_component *sel = entity->get_component<selectable_component>();
  if (sel != nullptr) {
    sel->sig_selected.connect(std::bind(&builder_component::on_selected, this, _1));
  }
  _particle_effect_component = entity->get_component<particle_effect_component>();
}

void builder_component::apply_template(luabind::object const &tmpl) {
  for (luabind::iterator it(tmpl), end; it != end; ++it) {
    if (it.key() == "BuildGroup") {
      _build_group = luabind::object_cast<std::string>(*it);
    }
  }
}

// this is called when our entity is selected/deselected, we need to show/hide the build window as appropriate.
void builder_component::on_selected(bool selected) {
  if (selected) {
    game::hud_build->show();
    game::hud_build->refresh(_entity, _build_group);
  } else {
    game::hud_build->hide();
  }
}

void builder_component::build(std::string name) {
  //game::hud_chat->add_line("Building: " + name + "...");

  entity_factory factory;
  _curr_building = factory.get_template(name);
  _time_to_build = 5.00f; // todo: get this from the template

  if (_particle_effect_component != nullptr) {
    _particle_effect_component->start_effect("building");
  }
}

bool builder_component::is_building() const {
  return !!_curr_building;
}

void builder_component::update(float dt) {
  if (_curr_building) {
    _time_to_build -= dt;

    if (_time_to_build <= 0.0f) {
      std::shared_ptr<entity> entity(_entity);
      ownable_component *our_ownable = entity->get_component<ownable_component>();
      if (our_ownable != 0 && our_ownable->is_local_or_ai_player()) {
        position_component *our_pos = entity->get_component<position_component>();
        if (our_pos != 0) {
          std::shared_ptr<game::create_entity_command> cmd(game::create_command<game::create_entity_command>());
          cmd->template_name = luabind::object_cast<std::string>(_curr_building["name"]);
          cmd->initial_position = our_pos->get_position();
          cmd->initial_goal = our_pos->get_position() + (our_pos->get_direction() * 3.0f);
          game::simulation_thread::get_instance()->post_command(cmd);
        }
      }

      _curr_building = luabind::object();
      _time_to_build = 0.0f;
      if (_particle_effect_component != nullptr) {
        _particle_effect_component->stop_effect("building");
      }
    }
  }
}

}

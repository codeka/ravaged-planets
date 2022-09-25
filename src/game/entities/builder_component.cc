#include <functional>

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
#include <game/simulation/player.h>

namespace ent {

using namespace std::placeholders;

// register the builder component with the entity_factory
ENT_COMPONENT_REGISTER("Builder", BuilderComponent);

BuilderComponent::BuilderComponent() {
}

BuilderComponent::~BuilderComponent() {
}

void BuilderComponent::initialize() {
  std::shared_ptr<Entity> Entity(entity_);
  SelectableComponent *sel = Entity->get_component<SelectableComponent>();
  if (sel != nullptr) {
    sel->sig_selected.connect(std::bind(&BuilderComponent::on_selected, this, _1));
  }
  _particle_effect_component = Entity->get_component<ParticleEffectComponent>();
}

void BuilderComponent::apply_template(luabind::object const &tmpl) {
//  for (luabind::iterator it(tmpl), end; it != end; ++it) {
//    if (it.key() == "BuildGroup") {
//      build_group_ = luabind::object_cast<std::string>(*it);
//    }
//  }
}

// this is called when our Entity is selected/deselected, we need to show/hide the build window as appropriate.
void BuilderComponent::on_selected(bool selected) {
  if (selected) {
    game::hud_build->show();
    game::hud_build->refresh(entity_, build_group_);
  } else {
    game::hud_build->hide();
  }
}

void BuilderComponent::build(std::string name) {
  //game::hud_chat->add_line("Building: " + name + "...");

  EntityFactory factory;
  queue_entry entry;
  entry.tmpl = factory.get_template(name);
  entry.time_to_build = 2.0f;// luabind::object_cast<float>(entry.tmpl["components"]["Buildable"]["TimeToBuild"]);
  entry.time_remaining = entry.time_to_build;
  entry.percent_complete = 0.0f;
  _build_queue.push(entry);

  if (_particle_effect_component != nullptr && _build_queue.size() == 1) {
    _particle_effect_component->start_effect("building");
  }
}

bool BuilderComponent::is_building() const {
  return !_build_queue.empty();
}

void BuilderComponent::update(float dt) {
  if (_build_queue.empty()) {
    return;
  }

  queue_entry &entry = _build_queue.front();
  entry.time_remaining -= dt;
  entry.percent_complete += (dt / entry.time_to_build) * 100.0f;
  if (entry.percent_complete >= 100.0f) {
    entry.percent_complete = 100.0f;

    // we've finished so actually create the Entity
    std::shared_ptr<Entity> Entity(entity_);
    OwnableComponent *our_ownable = Entity->get_component<OwnableComponent>();
    if (our_ownable != nullptr && our_ownable->is_local_or_ai_player()) {
      PositionComponent *our_pos = Entity->get_component<PositionComponent>();
      if (our_pos != nullptr) {
        std::shared_ptr<game::CreateEntityCommand> cmd(
            game::create_command<game::CreateEntityCommand>(our_ownable->get_owner()->get_player_no()));
        cmd->template_name = "test";// luabind::object_cast<std::string>(entry.tmpl["name"]);
        cmd->initial_position = our_pos->get_position();
        cmd->initial_goal = our_pos->get_position() + (our_pos->get_direction() * 3.0f);
        game::SimulationThread::get_instance()->post_command(cmd);
      }
    }

    // technically, we could probably start on part of the next entry this update, but for simplicity sake
    // we'll just wait for the next update.
    _build_queue.pop();
    if (_build_queue.empty()) {
      _particle_effect_component->stop_effect("building");
    }
  }
}

}

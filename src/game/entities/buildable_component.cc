
#include <game/entities/entity_factory.h>
#include <game/entities/buildable_component.h>

namespace ent {
ENT_COMPONENT_REGISTER("Buildable", BuildableComponent);

BuildableComponent::BuildableComponent() {
}

BuildableComponent::~BuildableComponent() {
}

void BuildableComponent::apply_template(fw::lua::Value tmpl) {
  build_group_ = tmpl["BuildGroup"];
}

}

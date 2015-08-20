#include <boost/foreach.hpp>

#include <game/entities/entity_factory.h>
#include <game/entities/buildable_component.h>

namespace ent {
ENT_COMPONENT_REGISTER("buildable", buildable_component);

buildable_component::buildable_component() {
}

buildable_component::~buildable_component() {
}

void buildable_component::apply_template(std::shared_ptr<entity_component_template> comp_template) {
  BOOST_FOREACH(auto &kvp, comp_template->properties) {
    if (kvp.first == "BuildGroup") {
      _build_group = kvp.second;
    }
  }

  entity_component::apply_template(comp_template);
}

}

#include <boost/foreach.hpp>

#include <game/entities/entity_factory.h>
#include <game/entities/buildable_component.h>

namespace ent {
ENT_COMPONENT_REGISTER("Buildable", buildable_component);

buildable_component::buildable_component() {
}

buildable_component::~buildable_component() {
}

void buildable_component::apply_template(luabind::object const &tmpl) {
//  for(luabind::iterator it(tmpl), end; it != end; ++it) {
//    if (it.key() == "BuildGroup") {
//      _build_group = luabind::object_cast<std::string>(*it);
//    }
//  }
}

}

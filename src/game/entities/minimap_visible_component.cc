
#include <game/entities/entity_factory.h>
#include <game/entities/minimap_visible_component.h>

namespace ent {
ENT_COMPONENT_REGISTER("minimap-visible", minimap_visible_component);

minimap_visible_component::minimap_visible_component() {
}

minimap_visible_component::~minimap_visible_component() {
}

}

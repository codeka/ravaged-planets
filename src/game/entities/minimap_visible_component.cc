
#include <game/entities/entity_factory.h>
#include <game/entities/minimap_visible_component.h>

namespace ent {
ENT_COMPONENT_REGISTER("MinimapVisible", minimap_visible_component);

minimap_visible_component::minimap_visible_component() {
}

minimap_visible_component::~minimap_visible_component() {
}

}

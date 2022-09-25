
#include <memory>

#include <framework/graphics.h>
#include <framework/exception.h>

#include <game/editor/editor_world.h>
#include <game/editor/editor_terrain.h>

namespace ed {
    
world_create::world_create() {
}

world_create::world_create(int width, int height) {
  terrain_ = create_terrain(width, height);
}

game::Terrain *world_create::create_terrain(int width, int length) {
  editor_terrain *et = new editor_terrain();
  et->create(width, length);
  et->initialize_splatt();
  return et;
}

//-------------------------------------------------------------------------

editor_world::editor_world(std::shared_ptr<game::WorldReader> reader) :
    World(reader) {
}

editor_world::~editor_world() {
}

void editor_world::initialize_entities() {
  // no entities!
}

void editor_world::initialize_pathing() {
  // no pathing
}

}

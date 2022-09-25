
#include <memory>

#include <framework/graphics.h>
#include <framework/exception.h>

#include <game/editor/editor_world.h>
#include <game/editor/editor_terrain.h>

namespace ed {
    
WorldCreate::WorldCreate() {
}

WorldCreate::WorldCreate(int width, int height) {
  terrain_ = create_terrain(width, height);
}

game::Terrain *WorldCreate::create_terrain(int width, int length) {
  EditorTerrain *et = new EditorTerrain();
  et->create(width, length);
  et->initialize_splatt();
  return et;
}

//-------------------------------------------------------------------------

EditorWorld::EditorWorld(std::shared_ptr<game::WorldReader> reader) :
    World(reader) {
}

EditorWorld::~EditorWorld() {
}

void EditorWorld::initialize_entities() {
  // no entities!
}

void EditorWorld::initialize_pathing() {
  // no pathing
}

}

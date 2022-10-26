
#include <memory>

#include <framework/graphics.h>
#include <framework/exception.h>

#include <game/editor/editor_world.h>
#include <game/editor/editor_terrain.h>

namespace ed {
    
WorldCreate::WorldCreate() {
}

WorldCreate::WorldCreate(int width, int height) {
  terrain_ = create_terrain(width, height, /*height_data*/ nullptr);
}

game::Terrain *WorldCreate::create_terrain(int width, int length, float* height_data) {
  EditorTerrain *et = new EditorTerrain(width, length, height_data);
  et->initialize();
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

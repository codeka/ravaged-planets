
#include <memory>

#include <framework/graphics.h>

#include <game/editor/editor_world.h>
#include <game/editor/editor_terrain.h>

namespace ed {
    
WorldCreate::WorldCreate() {
}

WorldCreate::WorldCreate(int width, int height) {
  auto terrain = create_terrain(width, height, /* height_data= */ nullptr);
  if (!terrain.ok()) {
    // TODO: make this a factory method instead so we can return the error
    fw::debug << "Error creating terrain: " << terrain.status() << std::endl;
  } else {
    terrain_ = *terrain;
  }
}

fw::StatusOr<std::shared_ptr<game::Terrain>> WorldCreate::create_terrain(
    int width, int length, float* height_data) {
  auto et = std::make_shared<EditorTerrain>(width, length, height_data);
  RETURN_IF_ERROR(et->initialize());
  et->initialize_splatt();
  return std::static_pointer_cast<game::Terrain>(et);
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

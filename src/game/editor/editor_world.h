#pragma once

#include <memory>

#include <game/world/world.h>
#include <game/world/world_reader.h>

namespace fw {
class Graphics;
}

namespace game {
class Terrain;
}

namespace ed {

// This is a specialization of world_reader that can create a brand new world from scratch, or load one as well.
class WorldCreate: public game::WorldReader {
protected:
  fw::StatusOr<std::shared_ptr<game::Terrain>> create_terrain(
        int width, int length, float* height_data) override;

public:
  WorldCreate();
  WorldCreate(int width, int height);
};

// The world in the editor is a bit different - no entities and so on...
class EditorWorld: public game::World {
protected:
  void initialize_entities() override;
  void initialize_pathing() override;

public:
  EditorWorld(std::shared_ptr<game::WorldReader> reader);
  virtual ~EditorWorld();

  void set_screenshot(fw::Bitmap bmp) {
    screenshot_ = bmp;
  }
};

}

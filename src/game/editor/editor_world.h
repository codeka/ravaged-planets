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

/** This is a specialization of world_reader that creates a brand new world from scratch. */
class WorldCreate: public game::WorldReader {
protected:
  virtual game::Terrain *create_terrain(int width, int length);

public:
  WorldCreate();
  WorldCreate(int width, int height);
};

// The world in the editor is a bit different - no entities and so on...
class EditorWorld: public game::World {
protected:
  virtual void initialize_entities();
  virtual void initialize_pathing();

public:
  EditorWorld(std::shared_ptr<game::WorldReader> reader);
  virtual ~EditorWorld();

  void set_screenshot(std::shared_ptr<fw::Bitmap> bmp) {
    screenshot_ = bmp;
  }
};

}

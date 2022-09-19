#pragma once

#include <memory>

#include <game/world/world.h>
#include <game/world/world_reader.h>

namespace fw {
class graphics;
}

namespace game {
class terrain;
}

namespace ed {

/** This is a specialization of world_reader that creates a brand new world from scratch. */
class world_create: public game::world_reader {
protected:
  virtual game::terrain *create_terrain(int width, int length);

public:
  world_create();
  world_create(int width, int height);
};

// The world in the editor is a bit different - no entities and so on...
class editor_world: public game::world {
protected:
  virtual void initialize_entities();
  virtual void initialize_pathing();

public:
  editor_world(std::shared_ptr<game::world_reader> reader);
  virtual ~editor_world();

  void set_screenshot(std::shared_ptr<fw::Bitmap> bmp) {
    _screenshot = bmp;
  }
};

}

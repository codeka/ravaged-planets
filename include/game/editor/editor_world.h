#pragma once

#include <memory>

#include <game/world/world.h>
#include <game/world/world_reader.h>

namespace fw {
class graphics;
}

namespace rp {
class terrain;
}

namespace ed {

// this is a specialization of world_reader that creates a brand new world from
// scratch. this, obviously, is only useful to the editor!
class world_create: public rp::world_reader {
private:
  int _width;
  int _height;

protected:
  virtual rp::terrain *create_terrain(int width, int length);

public:
  world_create();
  world_create(int width, int height);
};

// The world in the editor is a bit different - no entities and so on...
class editor_world: public rp::world {
protected:
  virtual void initialize_entities();
  virtual void initialize_pathing();

  std::shared_ptr<fw::bitmap> _screenshot;

public:
  editor_world(std::shared_ptr<rp::world_reader> reader);
  virtual ~editor_world();

  std::shared_ptr<fw::bitmap> get_screenshot() const {
    return _screenshot;
  }
  void set_screenshot(std::shared_ptr<fw::bitmap> bmp) {
    _screenshot = bmp;
  }
};

}

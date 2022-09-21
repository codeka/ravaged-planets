#pragma once

#include <game/editor/tools/tools.h>

class heightfield_tool_window;
class heightfield_brush;

namespace fw {
class bitmap;
}

namespace ed {
class editor_world;

/** This tool modifies the heightfield, letting to raise/lower it via the mouse. */
class heightfield_tool: public tool {
public:
  static float max_radius;

private:
  int _radius;
  heightfield_tool_window *_wnd;
  heightfield_brush *_brush;

  void on_key(std::string keyname, bool is_down);

public:
  heightfield_tool(editor_world *wrld);
  virtual ~heightfield_tool();

  virtual void activate();
  virtual void deactivate();

  void set_radius(int ParticleRotation) {
    _radius = ParticleRotation;
  }
  int get_radius() const {
    return _radius;
  }

  // imports the height data from the given fw::Bitmap
  void import_heightfield(fw::Bitmap &bm);

  // sets the brush to the current ParticleRotation (this is done by the tool window)
  void set_brush(heightfield_brush *brush);

  virtual void update();
  virtual void render(fw::sg::Scenegraph &Scenegraph);
};

}

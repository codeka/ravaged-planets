#pragma once

#include <game/editor/tools/tools.h>

class HeightfieldToolWindow;
class HeightfieldBrush;

namespace fw {
class bitmap;
}

namespace ed {
class EditorWorld;

// This tool modifies the heightfield, letting to raise/lower it via the mouse.
class HeightfieldTool: public Tool {
public:
  static float max_radius;

private:
  int radius_;
  HeightfieldToolWindow *wnd_;
  HeightfieldBrush *brush_;

  void on_key(std::string keyname, bool is_down);

public:
  HeightfieldTool(EditorWorld *wrld);
  virtual ~HeightfieldTool();

  virtual void activate();
  virtual void deactivate();

  void set_radius(int value) {
    radius_ = value;
  }
  int get_radius() const {
    return radius_;
  }

  // imports the height data from the given fw::Bitmap
  void import_heightfield(fw::Bitmap &bm);

  // sets the brush to the current ParticleRotation (this is done by the tool window)
  void set_brush(HeightfieldBrush *brush);

  virtual void update();
  virtual void render(fw::sg::Scenegraph &scenegraph);
};

}

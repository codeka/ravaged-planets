#pragma once

#include <game/editor/tools/tools.h>

class TextureToolWindow;

namespace fw {
namespace sg {
class Scenegraph;
}
}

namespace ed {
class EditorWorld;

/** This tool lets you draw the texture on the terrain. */
class TextureTool: public Tool {
public:
  static float max_radius;

private:
  int radius_;
  int _layer;
  TextureToolWindow *wnd_;
  bool _is_painting;

  void on_key(std::string keyname, bool is_down);
  uint32_t get_selected_splatt_mask();

public:
  TextureTool(EditorWorld *wrld);
  virtual ~TextureTool();

  virtual void activate();
  virtual void deactivate();

  void set_radius(int value) {
    radius_ = value;
  }
  int get_radius() const {
    return radius_;
  }
  void set_layer(int layer) {
    _layer = layer;
  }
  int get_layer() const {
    return _layer;
  }

  virtual void update();
  virtual void render(fw::sg::Scenegraph &scenegraph);
};

}


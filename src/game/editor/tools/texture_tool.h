#pragma once

#include <game/editor/tools/tools.h>

class texture_tool_window;

namespace fw {
namespace sg {
class scenegraph;
}
}

namespace ed {
class editor_world;

/** This tool lets you draw the texture on the terrain. */
class texture_tool: public tool {
public:
  static float max_radius;

private:
  int _radius;
  int _layer;
  texture_tool_window *_wnd;
  bool _is_painting;

  void on_key(std::string keyname, bool is_down);
  uint32_t get_selected_splatt_mask();

public:
  texture_tool(editor_world *wrld);
  virtual ~texture_tool();

  virtual void activate();
  virtual void deactivate();

  void set_radius(int value) {
    _radius = value;
  }
  int get_radius() const {
    return _radius;
  }
  void set_layer(int layer) {
    _layer = layer;
  }
  int get_layer() const {
    return _layer;
  }

  virtual void update();
  virtual void render(fw::sg::scenegraph &scenegraph);
};

}


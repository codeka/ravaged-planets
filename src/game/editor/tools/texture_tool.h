#pragma once

#include <game/editor/tools/tools.h>
#include <game/editor/tools/indicator_node.h>

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
  uint8_t layer_;
  TextureToolWindow *wnd_;
  bool is_painting_;

  std::shared_ptr<IndicatorNode> indicator_;

  void on_key(std::string keyname, bool is_down);

public:
  TextureTool(EditorWorld *wrld);
  virtual ~TextureTool();

  virtual void activate();
  virtual void deactivate();

  void set_radius(int value);
  int get_radius() const {
    return radius_;
  }
  void set_layer(uint8_t layer) {
    layer_ = layer;
  }
  uint8_t get_layer() const {
    return layer_;
  }

  virtual void update();
};

}


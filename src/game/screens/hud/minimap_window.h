#pragma once

#include <framework/gui/window.h>
#include <framework/math.h>
#include <framework/texture.h>
#include <framework/signals.h>

namespace fw {
class Shader;
class ShaderParameters;
class VertexBuffer;
class IndexBuffer;
namespace gui {
class Window;
}
}

namespace game {
class MinimapDrawable;

// The MinimapWindow shows a graphic with the current map and all the friendlies/enemies/etc
class MinimapWindow {
private:
  fw::gui::Window *wnd_;
  std::shared_ptr<fw::Texture> texture_;
  std::shared_ptr<MinimapDrawable> drawable_;
  float last_entity_display_update_;

  // this is fired when the camera is moved/rotated/etc - we have to update our matrix
  void on_camera_updated();
  fw::SignalConnection camera_updated_connection_;

  // this is called every now&then to update the display of entities on the map
  void update_entity_display();

  void update_drawable();

public:
  MinimapWindow();
  virtual ~MinimapWindow();

  void initialize();
  void update();
  void show();
  void hide();
};

extern MinimapWindow *hud_minimap;

}

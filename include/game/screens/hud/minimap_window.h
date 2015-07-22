#pragma once

#define BOOST_BIND_NO_PLACEHOLDERS // so it doesn't auto-include _1, _2 etc.
#include <boost/signals2.hpp>

#include <framework/gui/window.h>
#include <framework/vector.h>

namespace fw {
class shader;
class shader_parameters;
class vertex_buffer;
class index_buffer;
class texture;
namespace gui {
class window;
class widget;
}
}

namespace game {

// The minimap_window shows a graphic with the current map and all the friendlies/enemies/etc
class minimap_window {
private:
  fw::gui::window *_wnd;
  std::shared_ptr<fw::texture> _fg_texture;
  fw::matrix _m;
  fw::vector _map_size;
  fw::vector _map_centre;
  float _last_entity_display_update;

  // this is fired when the camera is moved/rotated/etc - we have to update our matrix
  void on_camera_updated();
  boost::signals2::connection _camera_updated_connection;

  // this is called every now&then to update the display of entities on the map
  void update_entity_display();

public:
  minimap_window();
  virtual ~minimap_window();

  void initialize();
  void show();
  void hide();
};

extern minimap_window *hud_minimap;

}

#include <boost/foreach.hpp>

#include <framework/shadows.h>
#include <framework/shader.h>
#include <framework/graphics.h>
#include <framework/texture.h>
#include <framework/framework.h>
#include <framework/timer.h>

namespace fw {

light_camera::light_camera() {
  set_projection_matrix(cml::constantsf::pi() / 8.0f, 1.0f, 200.0f, 500.0f);
}

light_camera::~light_camera() {
}

//---------------------------------------------------------------------------------------------------------
// this is a static list of shadow map textures, so we don't have create/destroy them
// over and over (as shadow_sources get created/destroyed)
static std::list<std::shared_ptr<Framebuffer>> g_shadowbuffers;

shadow_source::shadow_source() {
}

shadow_source::~shadow_source() {
  if (_shadowbuffer) {
    g_shadowbuffers.push_front(_shadowbuffer);
  }
}

void shadow_source::initialize(bool debug /*= false */) {
  if (g_shadowbuffers.empty()) {
    _shadowbuffer = std::shared_ptr<Framebuffer>(new Framebuffer());
    std::shared_ptr<fw::Texture> depth_texture(new Texture());
    depth_texture->create(1024, 1024, true);
    _shadowbuffer->set_depth_buffer(depth_texture);

    if (debug) {
      std::shared_ptr<fw::Texture> color_texture(new Texture());
      color_texture->create(1024, 1024, false);
      _shadowbuffer->set_color_buffer(color_texture);
    }
  } else {
    _shadowbuffer = g_shadowbuffers.front();
    g_shadowbuffers.pop_front();
  }
}

void shadow_source::begin_scene() {
  framework *frmwrk = fw::framework::get_instance();
  _camera.update(frmwrk->get_timer()->get_frame_time());
  frmwrk->get_graphics()->set_render_target(_shadowbuffer);
}

void shadow_source::end_scene() {
  framework *frmwrk = fw::framework::get_instance();

  // reset the render target and camera back to the "real" one
  frmwrk->get_graphics()->set_render_target(nullptr);
}

}

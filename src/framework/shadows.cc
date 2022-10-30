
#include <framework/shadows.h>
#include <framework/shader.h>
#include <framework/graphics.h>
#include <framework/texture.h>
#include <framework/framework.h>
#include <framework/timer.h>

namespace fw {

LightCamera::LightCamera() {
  set_projection_matrix(cml::constantsf::pi() / 8.0f, 1.0f, 200.0f, 500.0f);
}

LightCamera::~LightCamera() {
}

//---------------------------------------------------------------------------------------------------------
// this is a static list of shadow map textures, so we don't have create/destroy them over and over (as shadow_sources
// get created/destroyed)
static std::list<std::shared_ptr<Framebuffer>> g_shadowbuffers;

ShadowSource::ShadowSource() {
}

ShadowSource::~ShadowSource() {
  if (shadowbuffer_) {
    g_shadowbuffers.push_front(shadowbuffer_);
  }
}

void ShadowSource::initialize(bool debug /*= false */) {
  if (g_shadowbuffers.empty()) {
    shadowbuffer_ = std::shared_ptr<Framebuffer>(new Framebuffer());
    std::shared_ptr<fw::Texture> depth_texture(new Texture());
    depth_texture->create_depth(1024, 1024);
    shadowbuffer_->set_depth_buffer(depth_texture);

    if (debug) {
      std::shared_ptr<fw::Texture> color_texture(new Texture());
      color_texture->create(1024, 1024);
      shadowbuffer_->set_color_buffer(color_texture);
    }
  } else {
    shadowbuffer_ = g_shadowbuffers.front();
    g_shadowbuffers.pop_front();
  }
}

void ShadowSource::begin_scene() {
  Framework *frmwrk = fw::Framework::get_instance();
  camera_.update(frmwrk->get_timer()->get_update_time());
  frmwrk->get_graphics()->set_render_target(shadowbuffer_);
}

void ShadowSource::end_scene() {
  Framework *frmwrk = fw::Framework::get_instance();

  // reset the render target and camera back to the "real" one
  frmwrk->get_graphics()->set_render_target(nullptr);
}

}

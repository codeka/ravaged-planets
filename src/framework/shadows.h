#pragma once

#include <framework/camera.h>
#include <framework/shader.h>
#include <framework/texture.h>

namespace fw {

// This is a special camera implementation for rendering the shadow map. basically, we don't link to any Input
// controls, etc.
class LightCamera: public Camera {
public:
  LightCamera();
  virtual ~LightCamera();
};

// This class represents a "shadow source". It contains the camera we use to render the shadow texture, and the shadow
// texture itself. The graphics class has one instance of us per shadow source in the scene.
class ShadowSource: private boost::noncopyable {
private:
  LightCamera camera_;
  std::shared_ptr<Framebuffer> shadowbuffer_;

public:
  ShadowSource();
  ~ShadowSource();

  void initialize(bool debug = false);
  void destroy();

  // This should be called before you call graphics::begin_scene to set up this ShadowSource as the render target
  void begin_scene();

  // This should be called after you call graphics::end_scene to reset the render target.
  void end_scene();

  // gets the camera which you can use to "direct" the Light source
  LightCamera &get_camera() {
    return camera_;
  }

  // gets the actual framebuffer object
  std::shared_ptr<Framebuffer> get_shadowmap() {
    return shadowbuffer_;
  }
};

}

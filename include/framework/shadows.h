#pragma once

#include <framework/camera.h>

namespace fw {
class shader;
class texture;
namespace sg {
class scenegraph;
}

/**
 * This is a special camera implementation for rendering the shadow map. basically, we don't
 * link to any input controls, etc.
 */
class light_camera: public camera {
public:
  light_camera();
  virtual ~light_camera();
};

/**
 * This class represents a "shadow source". It contains the camera we use to render the shadow texture, and the shadow
 * texture itself. the graphics class has one instance of us per shadow source in the scene.
 */
class shadow_source: private boost::noncopyable {
private:
  light_camera _camera;
  std::shared_ptr<texture> _shadowmap;
  camera *_real_camera;

public:
  shadow_source();
  ~shadow_source();

  void initialize();
  void destroy();

  // this should be called before you call graphics::begin_scene to
  // set up this shadow_source as the render target
  void begin_scene();

  // this should be called after you call graphics::end_scene to
  // reset the render target
  void end_scene();

  // gets the camera which you can use to "direct" the light source
  light_camera &get_camera() {
    return _camera;
  }

  // gets the actual shadowmap texture
  std::shared_ptr<texture> get_shadowmap() {
    return _shadowmap;
  }
};

}

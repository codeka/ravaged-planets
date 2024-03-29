#pragma once

#include <list>
#include <memory>

#include <framework/graphics.h>
#include <framework/texture.h>
#include <framework/scenegraph.h>
#include <framework/shader.h>

struct RenderState;

namespace fw {
class Particle;
class ParticleManager;

// The ParticleRenderer is responsible for rendering particles. We create our own special Scenegraph Node that does
// the main work.
class ParticleRenderer : public fw::sg::ScenegraphCallback {
public:
  // This is the type of a list of particles. It must match the ParticleList type defined in ParticleManager.
  typedef std::list<std::weak_ptr<Particle>> ParticleList;

private:
  Graphics *graphics_;
  std::shared_ptr<Shader> shader_;
  std::shared_ptr<ShaderParameters> shader_params_;
  std::shared_ptr<Texture> color_texture_;
  ParticleManager *mgr_;
  int draw_frame_;

  void render_particles(RenderState &rs, float offset_x, float offset_z);
  bool add_particle(RenderState &rs, int base_index, std::shared_ptr<Particle>& p, float offset_x, float offset_z);

  // sort the particles so they're optimal for rendering
  void sort_particles(ParticleRenderer::ParticleList &particles);

public:
  ParticleRenderer(ParticleManager *mgr);
  ~ParticleRenderer();

  void initialize(Graphics *g);

  // ScenegraphCallback methods.
  void after_render(fw::sg::Scenegraph& scenegraph, float dt) override;
};

}

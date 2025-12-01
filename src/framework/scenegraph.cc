
#include <memory>

#include <framework/scenegraph.h>
#include <framework/graphics.h>
#include <framework/framework.h>
#include <framework/camera.h>
#include <framework/logging.h>
#include <framework/exception.h>
#include <framework/misc.h>
#include <framework/shader.h>
#include <framework/shadows.h>
#include <framework/texture.h>
#include <framework/timer.h>
#include <framework/gui/gui.h>

static std::shared_ptr<fw::Shader> shadow_shader;
static std::shared_ptr<fw::Shader> basic_shader;

static bool is_rendering_shadow = false;
static std::shared_ptr<fw::ShadowSource> shadowsrc;

static std::map<fw::sg::PrimitiveType, uint32_t> g_primitive_type_map;

static void ensure_primitive_type_map() {
  if (g_primitive_type_map.size() > 0)
    return;

  g_primitive_type_map[fw::sg::PrimitiveType::kLineStrip] = GL_LINE_STRIP;
  g_primitive_type_map[fw::sg::PrimitiveType::kLineList] = GL_LINES;
  g_primitive_type_map[fw::sg::PrimitiveType::kTriangleList] = GL_TRIANGLES;
  g_primitive_type_map[fw::sg::PrimitiveType::kTriangleStrip] = GL_TRIANGLE_STRIP;
}

namespace fw {

namespace sg {

//-------------------------------------------------------------------------------------
Light::Light() :
    cast_shadows_(false) {
}

Light::Light(fw::Vector const &pos, fw::Vector const &dir, bool cast_shadows) :
    pos_(pos), dir_(dir), cast_shadows_(cast_shadows) {
}

Light::Light(Light const &copy) :
    pos_(copy.pos_), dir_(copy.dir_), cast_shadows_(copy.cast_shadows_) {
}

Light::~Light() {
}

//-------------------------------------------------------------------------------------
Node::Node() :
    world_(fw::identity()), parent_(0), cast_shadows_(true), primitive_type_(PrimitiveType::kUnknownPrimitiveType),
    enabled_(true) {
}

Node::~Node() {
}

void Node::add_child(std::shared_ptr<Node> child) {
  FW_ENSURE_RENDER_THREAD();

  if (child->parent_) {
    child->parent_->remove_child(child);
  }

  child->parent_ = this;
  children_.push_back(child);
}

void Node::remove_child(std::shared_ptr<Node> child) {
  FW_ENSURE_RENDER_THREAD();

  auto it = std::find(children_.begin(), children_.end(), child);
  if (it != children_.end())
    children_.erase(it);
}

void Node::clear_children() {
  FW_ENSURE_RENDER_THREAD();

  children_.clear();
}

// Get the Shader file to use. if we don't have one defined, look at our parent and keep looking up at our parents
// until we find one.
std::shared_ptr<fw::Shader> Node::get_shader() const {
  std::shared_ptr<fw::Shader> shader = shader_;
  if (!shader) {
    Node *parent = parent_;
    while (!shader && parent != 0) {
      shader = parent->shader_;
      parent = parent->parent_;
    }
  }

  return shader;
}

void Node::render(Scenegraph *sg, fw::Matrix const &model_matrix /*= fw::identity()*/) {
  // if we're not a shadow caster, and we're rendering shadows, don't render the Node this time around
  if (is_rendering_shadow && !cast_shadows_) {
    return;
  }

  // if we're not enabled, don't do anything.
  if (!enabled_) {
    return;
  }

  fw::Matrix transform(world_ * model_matrix);
  if (vb_) {
    std::shared_ptr<fw::Shader> shader = get_shader();
    if (is_rendering_shadow) {
      shader = shadow_shader;
    }

    fw::CameraRenderState camera = sg->get_camera();
    if (!shader) {
      render_noshader(camera, transform);
    } else {
      render_shader(shader, camera, transform);
    }
  }

  // render the children as well (todo: pass transformations)
  for(auto &child_node : children_) {
    child_node->render(sg, transform);
  }
}

// this is called when we're rendering a given Shader
void Node::render_shader(
    std::shared_ptr<fw::Shader> shader, const fw::CameraRenderState& camera, fw::Matrix const &transform) {

  std::shared_ptr<fw::ShaderParameters> parameters;
  if (shader_params_) {
    parameters = shader_params_;
  } else {
    parameters = shader->CreateParameters();
  }

  // add the world_view and world_view_proj parameters as well as shadow parameters
  fw::Matrix worldview = camera.view;
  worldview = transform * worldview;
  fw::Matrix worldviewproj = worldview * camera.projection;

  parameters->set_matrix("worldviewproj", worldviewproj);
  parameters->set_matrix("worldview", worldview);
  parameters->set_matrix("proj", camera.projection);

  if (!is_rendering_shadow && shadowsrc) {
    fw::Matrix lightviewproj = transform * shadowsrc->get_camera().get_view_matrix();
    lightviewproj *= shadowsrc->get_camera().get_projection_matrix();

    fw::Matrix bias = fw::Matrix(
      mat4x4 {
        { 0.5f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.5f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.5f, 0.0f },
        { 0.5f, 0.5f, 0.5f, 1.0f }});

    parameters->set_matrix("lightviewproj", bias * lightviewproj);
    parameters->set_texture("shadow_map", shadowsrc->get_shadowmap()->get_depth_buffer());
  }

  vb_->begin();
  shader->Begin(parameters);
  if (ib_) {
    ib_->begin();
    glDrawElements(
        g_primitive_type_map[primitive_type_], ib_->get_num_indices(), GL_UNSIGNED_SHORT, nullptr);
    ib_->end();
  } else {
    glDrawArrays(g_primitive_type_map[primitive_type_], 0, vb_->get_num_vertices());
  }
  shader->End();
  vb_->end();
}

void Node::render_noshader(const fw::CameraRenderState& camera, fw::Matrix const &transform) {
  if (!basic_shader) {
    basic_shader = fw::Shader::CreateOrEmpty("basic.shader");
  }

  render_shader(basic_shader, camera, transform);
}

void Node::populate_clone(std::shared_ptr<Node> clone) {
  clone->cast_shadows_ = cast_shadows_;
  clone->primitive_type_ = primitive_type_;
  clone->vb_ = vb_;
  clone->ib_ = ib_;
  clone->shader_ = shader_;
  if (shader_params_)
    clone->shader_params_ = shader_params_->Clone();
  clone->parent_ = parent_;
  clone->world_ = world_;

  // clone the children as well!
  for(auto& child : children_) {
    clone->children_.push_back(child->clone());
  }
}

std::shared_ptr<Node> Node::clone() {
  std::shared_ptr<Node> clone(new Node());
  populate_clone(clone);
  return clone;
}

//-----------------------------------------------------------------------------------------

Scenegraph::Scenegraph()
    : clear_color_(fw::Color(1, 0, 0, 0)) {
}

Scenegraph::~Scenegraph() {
}


//-----------------------------------------------------------------------------------------

// Called on the update thread. Enqueues the given closure to run on the render thread. We'll pass it the scenegraph
// that you can update, or whatever is needed.
void ScenegraphManager::enqueue(std::function<void(Scenegraph&)> closure) {
  std::unique_lock<std::mutex> lock(mutex_);
  closures_[update_index_].push_back(closure);
}

// Called on the render thread, before rendering a frame. We'll run all of the enqueued closures.
void ScenegraphManager::before_render() {
  FW_ENSURE_RENDER_THREAD();

  int render_index;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (update_index_ == 0) {
      update_index_ = 1;
      render_index = 0;
    } else {
      update_index_ = 0;
      render_index = 1;
    }
  }

  auto& closures = closures_[render_index];
  for (auto& closure : closures) {
    closure(scenegraph_);
  }
  closures.clear();
}

Scenegraph& ScenegraphManager::get_scenegraph() {
  FW_ENSURE_RENDER_THREAD();
  return scenegraph_;
}

}
//-----------------------------------------------------------------------------------------
static const bool g_shadow_debug = true;

// renders the scene!
void render(sg::Scenegraph &scenegraph, std::shared_ptr<fw::Framebuffer> render_target /*= nullptr*/,
    bool render_gui /*= true*/) {
  ensure_primitive_type_map();

  Graphics *g = fw::Framework::get_instance()->get_graphics();
  Timer* timer = fw::Framework::get_instance()->get_timer();

  if (!shadow_shader) {
    shadow_shader = fw::Shader::CreateOrEmpty("shadow.shader");
  }

  // set up the shadow sources that we'll need to render from first to get the various shadows going.
  std::vector<std::shared_ptr<ShadowSource>> shadows;
  for (auto& light : scenegraph.get_lights()) {
    if (light->get_cast_shadows()) {
      std::shared_ptr<ShadowSource> shdwsrc(new ShadowSource());
      shdwsrc->initialize(g_shadow_debug);

      LightCamera &cam = shdwsrc->get_camera();
      cam.set_location(light->get_position());
      cam.set_direction(light->get_direction());

      shadows.push_back(shdwsrc);
    }
  }

  // render the shadowmap(s) first
  is_rendering_shadow = true;
  for(auto shadowsrc : shadows) {
    shadowsrc->begin_scene();
    scenegraph.push_camera(shadowsrc->get_camera().get_render_state());
    g->begin_scene();
    for(auto& node : scenegraph.get_nodes()) {
      node->render(&scenegraph);
    }
    g->end_scene();
    scenegraph.pop_camera();
    shadowsrc->end_scene();
  }
  is_rendering_shadow = false;

  if (render_target) {
    g->set_render_target(render_target);
  }

  // now, render the main scene
  g->begin_scene(scenegraph.get_clear_color());
  for(auto& node : scenegraph.get_nodes()) {
    node->render(&scenegraph);
  }

  scenegraph.call_after_render(timer->get_frame_time());

  // make sure the shadowsrc is empty
  std::shared_ptr<ShadowSource> debug_shadowsrc;
  if (g_shadow_debug && shadows.size() > 0) {
    debug_shadowsrc = shadows[0];
  }

  if (render_gui) {
    // render the GUI now
    g->before_gui();

    if (g_shadow_debug && debug_shadowsrc) {
      auto shader = Shader::Create("gui.shader");
      if (!shader.ok()) {
        fw::debug << "ERROR creating gui.shader: " << shader.status() << std::endl;
      } else {
        auto shader_params = (*shader)->CreateParameters();
        // TODO: recalculating this every time seems wasteful
        fw::Graphics *g = fw::Framework::get_instance()->get_graphics();
        fw::Matrix pos_transform;
        pos_transform = fw::projection_orthographic(
            0.0f, static_cast<float>(g->get_width()),
            static_cast<float>(g->get_height()), 0.0f, 1.0f, -1.0f);
        pos_transform = fw::scale(fw::Vector(200.0f, 200.0f, 0.0f))
            * fw::translation(fw::Vector(10.0f, 10.0f, 0))
            * pos_transform;
        shader_params->set_matrix("pos_transform", pos_transform);
        shader_params->set_matrix("uv_transform", fw::identity());
        shader_params->set_texture(
            "texsampler", debug_shadowsrc->get_shadowmap()->get_color_buffer());

        std::shared_ptr<VertexBuffer> vb = VertexBuffer::create<vertex::xyz_uv>();
        fw::vertex::xyz_uv vertices[4];
        vertices[0] = fw::vertex::xyz_uv(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        vertices[1] = fw::vertex::xyz_uv(0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
        vertices[2] = fw::vertex::xyz_uv(1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        vertices[3] = fw::vertex::xyz_uv(1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
        vb->set_data(4, vertices);

        auto ib = std::make_shared<fw::IndexBuffer>();
        uint16_t indices[4];
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;
        indices[3] = 3;
        ib->set_data(4, indices);

        vb->begin();
        ib->begin();
        (*shader)->Begin(shader_params);
        glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr);
        (*shader)->End();
        ib->end();
        vb->end();
      }
    }

    Framework::get_instance()->get_gui()->render();
    g->after_gui();
  }

  g->end_scene();
  if (render_target) {
    g->set_render_target(nullptr);
  } else {
    g->present();
  }
}
}

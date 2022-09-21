#include <boost/foreach.hpp>

#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/shader.h>
#include <framework/texture.h>
#include <framework/scenegraph.h>
#include <framework/vector.h>
#include <game/entities/entity.h>
#include <game/entities/entity_factory.h>
#include <game/entities/selectable_component.h>
#include <game/entities/position_component.h>
#include <game/entities/ownable_component.h>
#include <game/world/world.h>
#include <game/world/terrain.h>

namespace ent {

static std::shared_ptr<fw::Shader> shader_;
static std::shared_ptr<fw::VertexBuffer> vb_;
static std::shared_ptr<fw::IndexBuffer> ib_;

// register the selectable component with the entity_factory
ENT_COMPONENT_REGISTER("Selectable", selectable_component);

selectable_component::selectable_component() :
    _is_selected(false), _selection_radius(2.0f), _is_highlighted(false), _ownable(nullptr) {
}

selectable_component::~selectable_component() {
}

void selectable_component::initialize() {
  // grab a reference to the ownable component of our entity so we can refer to it
  // later on.
  std::shared_ptr<entity> entity(_entity);
  _ownable = entity->get_component<ownable_component>();
}

// this is called when we start up, and also when our device is reset. we need to populate
// the vertex buffer and index buffer
void selectable_component::populate_buffers() {
  std::shared_ptr<fw::VertexBuffer> vb = fw::VertexBuffer::create<fw::vertex::xyz_uv>();
  fw::vertex::xyz_uv vertices[4] = {
      fw::vertex::xyz_uv(-1.0f, 0.0f, -1.0f, 0.0f, 0.0f),
      fw::vertex::xyz_uv(-1.0f, 0.0f, 1.0f, 0.0f, 1.0f),
      fw::vertex::xyz_uv(1.0f, 0.0f, 1.0f, 1.0f, 1.0f),
      fw::vertex::xyz_uv(1.0f, 0.0f, -1.0f, 1.0f, 0.0f)
  };
  vb->set_data(4, vertices);
  vb_ = vb;

  std::shared_ptr<fw::IndexBuffer> ib(new fw::IndexBuffer());
  uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };
  ib->set_data(6, indices);
  ib_ = ib;

  if (!shader_) {
    shader_ = fw::Shader::create("selection.shader");
  }
}

void selectable_component::set_is_selected(bool selected) {
  _is_selected = selected;
  sig_selected(selected);
}

void selectable_component::apply_template(luabind::object const &tmpl) {
//  for (luabind::iterator it(tmpl), end; it != end; ++it) {
//    if (it.key() == "SelectionRadius") {
//      set_selection_radius(luabind::object_cast<float>(*it));
//    }
//  }
}

void selectable_component::set_selection_radius(float ParticleRotation) {
  _selection_radius = ParticleRotation;
}

void selectable_component::highlight(fw::Color const &col) {
  _is_highlighted = true;
  _highlight_color = col;
}

void selectable_component::unhighlight() {
  _is_highlighted = false;
}

void selectable_component::render(fw::sg::Scenegraph &Scenegraph, fw::Matrix const &transform) {
  if (!vb_) {
    populate_buffers();
  }

  bool draw = false;
  fw::Color col(1, 1, 1);
  if (_is_selected) {
    draw = true;
    col = fw::Color(1, 1, 1);
  } else if (_is_highlighted) {
    draw = true;
    col = _highlight_color;
  }

  if (!draw)
    return;

  std::shared_ptr<entity> entity(_entity);
  position_component *pos = entity->get_component<position_component>();
  if (pos != 0) {
    std::shared_ptr<fw::ShaderParameters> shader_params = shader_->create_parameters();
    shader_params->set_color("selection_color", col);

    fw::Matrix m = pos->get_transform() * transform;
    m *= fw::translation(fw::Vector(0.0f, 0.2f, 0.0f)); // lift it off the ground a bit
    m = fw::scale(_selection_radius) * m; // scale it to the size of our selection radius

    std::shared_ptr<fw::sg::Node> Node(new fw::sg::Node());
    Node->set_vertex_buffer(vb_);
    Node->set_index_buffer(ib_);
    Node->set_shader(shader_);
    Node->set_shader_parameters(shader_params);
    Node->set_world_matrix(m);
    Node->set_primitive_type(fw::sg::PrimitiveType::kTriangleList);
    Node->set_cast_shadows(false);
    Scenegraph.add_node(Node);
  }
}
}

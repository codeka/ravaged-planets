
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
ENT_COMPONENT_REGISTER("Selectable", SelectableComponent);

SelectableComponent::SelectableComponent() :
    is_selected_(false), selection_radius_(2.0f), is_highlighted_(false), ownable_(nullptr) {
}

SelectableComponent::~SelectableComponent() {
}

void SelectableComponent::initialize() {
  // grab a reference to the ownable component of our Entity so we can refer to it
  // later on.
  std::shared_ptr<Entity> Entity(entity_);
  ownable_ = Entity->get_component<OwnableComponent>();
}

// this is called when we start up, and also when our device is reset. we need to populate
// the vertex buffer and index buffer
void SelectableComponent::populate_buffers() {
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

void SelectableComponent::set_is_selected(bool selected) {
  is_selected_ = selected;
  sig_selected(selected);
}

void SelectableComponent::apply_template(fw::lua::Value tmpl) {
  if (tmpl.has_key("SelectionRadius")) {
    set_selection_radius(tmpl["SelectionRadius"]);
  }
}

void SelectableComponent::set_selection_radius(float value) {
  selection_radius_ = value;
}

void SelectableComponent::highlight(fw::Color const &col) {
  is_highlighted_ = true;
  highlight_color_ = col;
}

void SelectableComponent::unhighlight() {
  is_highlighted_ = false;
}

void SelectableComponent::render(fw::sg::Scenegraph &Scenegraph, fw::Matrix const &transform) {
  if (!vb_) {
    populate_buffers();
  }

  bool draw = false;
  fw::Color col(1, 1, 1);
  if (is_selected_) {
    draw = true;
    col = fw::Color(1, 1, 1);
  } else if (is_highlighted_) {
    draw = true;
    col = highlight_color_;
  }

  if (!draw)
    return;

  std::shared_ptr<Entity> Entity(entity_);
  PositionComponent *pos = Entity->get_component<PositionComponent>();
  if (pos != 0) {
    std::shared_ptr<fw::ShaderParameters> shader_params = shader_->create_parameters();
    shader_params->set_color("selection_color", col);

    fw::Matrix m = pos->get_transform() * transform;
    m *= fw::translation(fw::Vector(0.0f, 0.2f, 0.0f)); // lift it off the ground a bit
    m = fw::scale(selection_radius_) * m; // scale it to the size of our selection radius

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

#include <game/entities/selectable_component.h>

#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/shader.h>
#include <framework/texture.h>
#include <framework/scenegraph.h>
#include <framework/math.h>

#include <game/entities/entity.h>
#include <game/entities/entity_factory.h>
#include <game/entities/mesh_component.h>
#include <game/entities/ownable_component.h>
#include <game/entities/position_component.h>
#include <game/world/world.h>
#include <game/world/terrain.h>

namespace ent {

namespace {
// The global scenegraph now we use for all selectable components. We clone this node and attach it to the main entity
// node, so it would share a vertex/index buffer and shader, but this way we can support multiple selected entities
// with ease.
std::shared_ptr<fw::sg::Node> g_node;

std::shared_ptr<fw::sg::Node> create_node() {
  std::shared_ptr<fw::VertexBuffer> vb = fw::VertexBuffer::create<fw::vertex::xyz_uv>();
  fw::vertex::xyz_uv vertices[4] = {
      fw::vertex::xyz_uv(-1.0f, 0.0f, -1.0f, 0.0f, 0.0f),
      fw::vertex::xyz_uv(-1.0f, 0.0f, 1.0f, 0.0f, 1.0f),
      fw::vertex::xyz_uv(1.0f, 0.0f, 1.0f, 1.0f, 1.0f),
      fw::vertex::xyz_uv(1.0f, 0.0f, -1.0f, 1.0f, 0.0f)
  };
  vb->set_data(4, vertices);

  std::shared_ptr<fw::IndexBuffer> ib(new fw::IndexBuffer());
  uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };
  ib->set_data(6, indices);

  auto shader = fw::Shader::create("selection.shader");
  std::shared_ptr<fw::ShaderParameters> shader_params = shader->create_parameters();
  shader_params->set_color("selection_color", fw::Color::WHITE());

  auto node = std::make_shared<fw::sg::Node>();
  node->set_vertex_buffer(vb);
  node->set_index_buffer(ib);
  node->set_shader(shader);
  node->set_shader_parameters(shader_params);
  node->set_primitive_type(fw::sg::PrimitiveType::kTriangleList);
  node->set_cast_shadows(false);
  return node;
}
}

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
  auto entity = entity_.lock();
  ownable_ = entity->get_component<OwnableComponent>();
  mesh_ = entity->get_component<MeshComponent>();
  pos_ = entity->get_component<PositionComponent>();
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

void SelectableComponent::update(float dt) {
  if (mesh_ == nullptr || pos_ == nullptr) {
    return;
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

  if (!draw) {
    if (sg_node_) {
      sg_node_->set_enabled(false);
    }
    return;
  }

  fw::Matrix m = fw::translation(fw::Vector(0.0f, 0.2f, 0.0f)); // lift it off the ground a bit
  m = fw::scale(selection_radius_) * m; // scale it to the size of our selection radius

  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
    [&sg_node = sg_node_, mesh = mesh_, col, m](fw::sg::Scenegraph&) {
      if (!g_node) {
        g_node = create_node();
      }
      if (!sg_node) {
        sg_node = g_node->clone();
      }
      sg_node->set_enabled(true);

      if (sg_node->get_parent() == nullptr) {
        mesh->get_sg_node()->add_child(sg_node);
      }

      sg_node->set_world_matrix(m);

      auto& shader_params = sg_node->get_shader_parameters();
      shader_params->set_color("selection_color", col);
    });
}
}

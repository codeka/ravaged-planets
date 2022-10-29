#include <game/editor/tools/indicator_node.h>

#include <framework/framework.h>

namespace ed {

IndicatorNode::IndicatorNode(EditorTerrain* terrain)
  : mgr_(fw::Framework::get_instance()->get_scenegraph_manager()), terrain_(terrain) {
}

IndicatorNode::~IndicatorNode() {
}

void IndicatorNode::set_radius(float radius) {
  mgr_->enqueue([radius, this](fw::sg::Scenegraph&) {
    radius_ = radius;
    dirty_ = true;
  });
}

void IndicatorNode::set_center(fw::Vector center) {
  mgr_->enqueue([center, this](fw::sg::Scenegraph&) {
    center_ = center;
    dirty_ = true;
    });
}

void IndicatorNode::render(fw::sg::Scenegraph* sg, fw::Matrix const& model_matrix /*= fw::identity()*/) {
  if (!initialized_) {
    set_primitive_type(fw::sg::PrimitiveType::kLineStrip);
    std::shared_ptr<fw::Shader> shader = fw::Shader::create("basic.shader");
    std::shared_ptr<fw::ShaderParameters> shader_params = shader->create_parameters();
    shader_params->set_program_name("notexture");
    set_shader(shader);
    set_shader_parameters(shader_params);

    initialized_ = true;
  }

  if (dirty_) {
    // the number of segments is basically the diameter of our circle. That means
    // we'll have one segment per unit, approximately.
    const int num_segments = std::max(8, (int)(2.0f * M_PI * radius_));

    std::shared_ptr<fw::VertexBuffer> vb = fw::VertexBuffer::create<fw::vertex::xyz_c>();
    fw::vertex::xyz_c* vertices = new fw::vertex::xyz_c[num_segments + 1];
    for (int i = 0; i < num_segments; i++) {
      float factor = 2.0f * (float)M_PI * (i / (float)num_segments);
      vertices[i].x = center_[0] + radius_ * sin(factor);
      vertices[i].z = center_[2] + radius_ * cos(factor);
      vertices[i].y = terrain_->get_height(vertices[i].x, vertices[i].z) + 0.5f;
      vertices[i].color = fw::Color(1, 1, 1).to_rgba();
    }
    vertices[num_segments].x = vertices[0].x;
    vertices[num_segments].y = vertices[0].y;
    vertices[num_segments].z = vertices[0].z;
    vertices[num_segments].color = fw::Color(1, 1, 1).to_rgba();
    vb->set_data(num_segments + 1, vertices);
    delete[] vertices;

    std::shared_ptr<fw::IndexBuffer> ib = std::shared_ptr<fw::IndexBuffer>(new fw::IndexBuffer());
    uint16_t* indices = new uint16_t[num_segments + 1];
    for (int i = 0; i < num_segments + 1; i++) {
      indices[i] = i;
    }
    ib->set_data(num_segments + 1, indices);
    delete[] indices;

    set_vertex_buffer(vb);
    set_index_buffer(ib);
    dirty_ = false;
  }

  fw::sg::Node::render(sg, model_matrix);
}


}  // namespace ed

#include <framework/framework.h>
#include <framework/input.h>
#include <framework/scenegraph.h>
#include <framework/shader.h>
#include <framework/graphics.h>
#include <game/application.h>
#include <game/world/world.h>
#include <game/editor/editor_terrain.h>
#include <game/editor/editor_screen.h>
#include <game/editor/editor_world.h>
#include <game/editor/tools/tools.h>

using namespace std::placeholders;

namespace ed {

tool::tool(editor_world *wrld) :
    world_(wrld), _terrain(nullptr), _editor(nullptr) {
}

tool::~tool() {
}

void tool::activate() {
  game::Application *app = dynamic_cast<game::Application *>(fw::Framework::get_instance()->get_app());
  _editor = dynamic_cast<editor_screen *>(app->get_screen_stack()->get_active_screen());
  _terrain = dynamic_cast<editor_terrain *>(world_->get_terrain());
}

void tool::deactivate() {
  // Loop through the keybind tokens and unbind them all
  fw::Input *Input = fw::Framework::get_instance()->get_input();
  std::for_each(_keybind_tokens.begin(), _keybind_tokens.end(), std::bind(&fw::Input::unbind_key, Input, _1));
}

void tool::update() {
}

void tool::render(fw::sg::Scenegraph &) {
}

// This is used by a number of of the tools for giving a basic indication of it's area of effect.
void draw_circle(fw::sg::Scenegraph &Scenegraph, game::Terrain *terrain, fw::Vector const &center, float radius) {
  // the number of segments is basically the diameter of our circle. That means
  // we'll have one segment per unit, approximately.
  int num_segments = (int) (2.0f * M_PI * radius);

  // at least 8 segments, though...
  if (num_segments < 8)
    num_segments = 8;

  std::shared_ptr<fw::VertexBuffer> vb = fw::VertexBuffer::create<fw::vertex::xyz_c>();
  fw::vertex::xyz_c *vertices = new fw::vertex::xyz_c[num_segments + 1];
  for (int i = 0; i < num_segments; i++) {
    float factor = 2.0f * (float) M_PI * (i / (float) num_segments);
    vertices[i].x = center[0] + radius * sin(factor);
    vertices[i].z = center[2] + radius * cos(factor);
    vertices[i].y = terrain->get_height(vertices[i].x, vertices[i].z) + 0.5f;
    vertices[i].color = fw::Color(1, 1, 1).to_rgba();
  }
  vertices[num_segments].x = vertices[0].x;
  vertices[num_segments].y = vertices[0].y;
  vertices[num_segments].z = vertices[0].z;
  vertices[num_segments].color = fw::Color(1, 1, 1).to_rgba();
  vb->set_data(num_segments + 1, vertices);
  delete[] vertices;

  std::shared_ptr<fw::IndexBuffer> ib = std::shared_ptr<fw::IndexBuffer>(new fw::IndexBuffer());
  uint16_t *indices = new uint16_t[num_segments + 1];
  for (int i = 0; i < num_segments + 1; i++) {
    indices[i] = i;
  }
  ib->set_data(num_segments + 1, indices);
  delete[] indices;

  std::shared_ptr<fw::sg::Node> Node(new fw::sg::Node());
  Node->set_vertex_buffer(vb);
  Node->set_index_buffer(ib);
  Node->set_primitive_type(fw::sg::PrimitiveType::kLineStrip);
  std::shared_ptr<fw::Shader> Shader = fw::Shader::create("basic.shader");
  std::shared_ptr<fw::ShaderParameters> shader_params = Shader->create_parameters();
  shader_params->set_program_name("notexture");
  Node->set_shader(Shader);
  Node->set_shader_parameters(shader_params);
  Scenegraph.add_node(Node);
}

//-------------------------------------------------------------------------
typedef std::map<std::string, create_tool_fn> tool_map;
static tool_map *g_tools = nullptr;

tool_factory_registrar::tool_factory_registrar(std::string const &name, create_tool_fn fn) {
  if (g_tools == 0) {
    g_tools = new tool_map();
  }

  (*g_tools)[name] = fn;
}

tool *tool_factory::create_tool(std::string const &name, editor_world *world) {
  if (g_tools == nullptr)
    return nullptr;

  tool_map::iterator it = g_tools->find(name);
  if (it == g_tools->end())
    return nullptr;

  return (*it).second(world);
}

}

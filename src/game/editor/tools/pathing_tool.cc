
#include <functional>

#include <framework/framework.h>
#include <framework/camera.h>
#include <framework/graphics.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/checkbox.h>
#include <framework/gui/gui.h>
#include <framework/gui/widget.h>
#include <framework/gui/window.h>
#include <framework/input.h>
#include <framework/model.h>
#include <framework/model_manager.h>
#include <framework/scenegraph.h>
#include <framework/shader.h>
#include <framework/paths.h>
#include <framework/path_find.h>

#include <game/editor/tools/pathing_tool.h>
#include <game/editor/windows/main_menu.h>
#include <game/editor/editor_terrain.h>
#include <game/editor/editor_world.h>
#include <game/world/terrain_helper.h>

static int PATCH_SIZE = 32; // our patches are independent of the terrain patches

using namespace std::placeholders;
using namespace fw::gui;

//-----------------------------------------------------------------------------

enum IDS {
  START_ID,
  END_ID,
};

class PathingToolWindow {
private:
  Window *wnd_;
  ed::PathingTool *tool_;

  bool on_start_click(Widget *w);
  bool on_end_click(Widget *w);
  bool on_simplify_click(Widget *w);

public:
  PathingToolWindow(ed::PathingTool *Tool);
  virtual ~PathingToolWindow();

  void show();
  void hide();
};

PathingToolWindow::PathingToolWindow(ed::PathingTool *tool) :
    tool_(tool), wnd_(nullptr) {
  wnd_ = Builder<Window>(px(10), px(30), px(100), px(94)) << Window::background("frame")
      << (Builder<Button>(px(4), px(4), sum(pct(100), px(-8)), px(30)) << Button::text("Start") << Widget::id(START_ID)
          << Widget::click(std::bind(&PathingToolWindow::on_start_click, this, _1)))
      << (Builder<Button>(px(4), px(38), sum(pct(100), px(-8)), px(30)) << Button::text("End") << Widget::id(END_ID)
          << Widget::click(std::bind(&PathingToolWindow::on_end_click, this, _1)))
      << (Builder<Checkbox>(px(4), px(72), sum(pct(100), px(-8)), px(18)) << Checkbox::text("Simplify")
          << Widget::click(std::bind(&PathingToolWindow::on_simplify_click, this, _1)));
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);
}

PathingToolWindow::~PathingToolWindow() {
  fw::Framework::get_instance()->get_gui()->detach_widget(wnd_);
}

void PathingToolWindow::show() {
  wnd_->set_visible(true);
}

void PathingToolWindow::hide() {
  wnd_->set_visible(false);
}

bool PathingToolWindow::on_start_click(Widget *w) {
  ed::statusbar->set_message("Set test start...");
  tool_->set_test_start();
  wnd_->find<Button>(START_ID)->set_pressed(true);
  wnd_->find<Button>(END_ID)->set_pressed(false);
  return true;
}

bool PathingToolWindow::on_end_click(Widget *w) {
  ed::statusbar->set_message("Set test end...");
  tool_->set_test_end();
  wnd_->find<Button>(START_ID)->set_pressed(false);
  wnd_->find<Button>(END_ID)->set_pressed(true);
  return true;
}

bool PathingToolWindow::on_simplify_click(Widget *w) {
  tool_->set_simplify(dynamic_cast<Checkbox *>(w)->is_checked());
  return true;
}

//-----------------------------------------------------------------------------

class CollisionPatch {
private:
  int _patch_x, _patch_z;
  static std::shared_ptr<fw::IndexBuffer> ib_;
  std::shared_ptr<fw::VertexBuffer> vb_;

public:
  void bake(std::vector<bool> &data, float *heights, int width, int length, int patch_x, int patch_z);

  void render(fw::sg::Scenegraph &Scenegraph, fw::Matrix const &world);
};

std::shared_ptr<fw::IndexBuffer> CollisionPatch::ib_;
std::shared_ptr<fw::VertexBuffer> current_path_vb;
std::shared_ptr<fw::IndexBuffer> current_path_ib;

void CollisionPatch::bake(std::vector<bool> &data, float *heights, int width, int length, int patch_x, int patch_z) {
  if (!ib_) {
    std::vector<uint16_t> indices;
    game::generate_terrain_indices_wireframe(indices, PATCH_SIZE);

    ib_ = std::shared_ptr<fw::IndexBuffer>(new fw::IndexBuffer());
    ib_->set_data(indices.size(), &indices[0]);
  }

  std::vector<fw::vertex::xyz_c> vertices((PATCH_SIZE + 1) * (PATCH_SIZE + 1));
  for (int z = 0; z <= PATCH_SIZE; z++) {
    for (int x = 0; x <= PATCH_SIZE; x++) {
      int ix = fw::constrain((patch_x * PATCH_SIZE) + x, width);
      int iz = fw::constrain((patch_z * PATCH_SIZE) + z, length);

      bool passable = data[(iz * width) + ix];
      fw::Color color(passable ? fw::Color(0.1f, 1.0f, 0.1f) : fw::Color(1.0f, 0.1f, 0.1f));

      int index = z * (PATCH_SIZE + 1) + x;
      vertices[index] = fw::vertex::xyz_c(x, heights[iz * width + ix] + 0.1f, z, color);
    }
  }

  vb_ = fw::VertexBuffer::create<fw::vertex::xyz_c>();
  vb_->set_data(vertices.size(), vertices.data());
}

void CollisionPatch::render(fw::sg::Scenegraph &Scenegraph, fw::Matrix const &world) {
  std::shared_ptr<fw::sg::Node> Node(new fw::sg::Node());
  Node->set_world_matrix(world);

  // we have to set up the Scenegraph Node with these manually
  Node->set_vertex_buffer(vb_);
  Node->set_index_buffer(ib_);
  Node->set_primitive_type(fw::sg::PrimitiveType::kLineList);
  std::shared_ptr<fw::Shader> Shader = fw::Shader::create("basic.shader");
  std::shared_ptr<fw::ShaderParameters> shader_params = Shader->create_parameters();
  shader_params->set_program_name("notexture");
  Node->set_shader(Shader);
  Node->set_shader_parameters(shader_params);

  Scenegraph.add_node(Node);
}

//-----------------------------------------------------------------------------

namespace ed {
REGISTER_TOOL("pathing", PathingTool);

PathingTool::PathingTool(EditorWorld *wrld) :
    Tool(wrld), start_set_(false), end_set_(false), test_mode_(kTestNone), simplify_(true) {
  wnd_ = new PathingToolWindow(this);
  marker_ = fw::Framework::get_instance()->get_model_manager()->get_model("marker");
}

PathingTool::~PathingTool() {
  delete wnd_;
}

void PathingTool::activate() {
  Tool::activate();
  wnd_->show();

  int width = get_terrain()->get_width();
  int length = get_terrain()->get_length();
  collision_data_.resize(width * length);
  get_terrain()->build_collision_data(collision_data_);

  patches_.resize((width / PATCH_SIZE) * (length / PATCH_SIZE));

  std::shared_ptr<fw::TimedPathFind> pf(
      new fw::TimedPathFind(get_terrain()->get_width(), get_terrain()->get_length(), collision_data_));
  path_find_ = pf;

  fw::Input *inp = fw::Framework::get_instance()->get_input();
  keybind_tokens_.push_back(
      inp->bind_key("Left-Mouse", fw::InputBinding(std::bind(&PathingTool::on_key, this, _1, _2))));
}

void PathingTool::deactivate() {
  Tool::deactivate();
  wnd_->hide();
}

int get_patch_index(int patch_x, int patch_z, int patches_width, int patches_length, int *new_patch_x,
    int *new_patch_z) {
  patch_x = fw::constrain(patch_x, patches_width);
  patch_z = fw::constrain(patch_z, patches_length);

  if (new_patch_x != 0)
    *new_patch_x = patch_x;
  if (new_patch_z != 0)
    *new_patch_z = patch_z;

  return patch_z * patches_width + patch_x;
}

void PathingTool::render(fw::sg::Scenegraph &Scenegraph) {
  // we want to render the patches centred on where the camera is looking
  fw::Camera *camera = fw::Framework::get_instance()->get_camera();
  fw::Vector cam_loc = camera->get_position();
  fw::Vector cam_dir = camera->get_direction();
  fw::Vector location = get_terrain()->get_cursor_location(cam_loc, cam_dir);

  int centre_patch_x = (int) (location[0] / PATCH_SIZE);
  int centre_patch_z = (int) (location[2] / PATCH_SIZE);

  int patch_width = get_terrain()->get_width() / PATCH_SIZE;
  int patch_length = get_terrain()->get_length() / PATCH_SIZE;
  for (int patch_z = centre_patch_z - 1; patch_z <= centre_patch_z + 1; patch_z++) {
    for (int patch_x = centre_patch_x - 1; patch_x <= centre_patch_x + 1; patch_x++) {
      int new_patch_x, new_patch_z;
      int patch_index = get_patch_index(patch_x, patch_z, patch_width, patch_length, &new_patch_x, &new_patch_z);

      if (!patches_[patch_index])
        patches_[patch_index] = bake_patch(new_patch_x, new_patch_z);

      fw::Matrix world = fw::translation(static_cast<float>(patch_x * PATCH_SIZE), 0,
          static_cast<float>(patch_z * PATCH_SIZE));

      patches_[patch_index]->render(Scenegraph, world);
    }
  }

  if (start_set_) {
    fw::Matrix loc(fw::translation(start_pos_));
    marker_->set_color(fw::Color(1, 0.1f, 1, 0.1f));
    marker_->render(Scenegraph, loc);
  }

  if (end_set_) {
    fw::Matrix loc(fw::translation(end_pos_));
    marker_->set_color(fw::Color(1, 1, 0.1f, 0.1f));
    marker_->render(Scenegraph, loc);
  }

  if (current_path_vb) {
    std::shared_ptr<fw::sg::Node> sgnode(new fw::sg::Node());
    sgnode->set_vertex_buffer(current_path_vb);
    sgnode->set_index_buffer(current_path_ib);
    sgnode->set_cast_shadows(false);
    sgnode->set_primitive_type(fw::sg::PrimitiveType::kLineList);
    std::shared_ptr<fw::Shader> Shader = fw::Shader::create("basic.shader");
    std::shared_ptr<fw::ShaderParameters> shader_params = Shader->create_parameters();
    shader_params->set_program_name("notexture");
    sgnode->set_shader(Shader);
    sgnode->set_shader_parameters(shader_params);

    Scenegraph.add_node(sgnode);
  }
}

std::shared_ptr<CollisionPatch> PathingTool::bake_patch(int patch_x, int patch_z) {
  std::shared_ptr<CollisionPatch> patch(new CollisionPatch());
  patch->bake(collision_data_, get_terrain()->get_height_data(), get_terrain()->get_width(),
      get_terrain()->get_length(), patch_x, patch_z);
  return patch;
}

void PathingTool::set_simplify(bool ParticleRotation) {
  simplify_ = ParticleRotation;
  find_path();
}

void PathingTool::set_test_start() {
  test_mode_ = kTestStart;
}

void PathingTool::set_test_end() {
  test_mode_ = kTestEnd;
}

void PathingTool::stop_testing() {
  test_mode_ = kTestNone;
}

void PathingTool::on_key(std::string keyname, bool is_down) {
  if (test_mode_ == kTestNone) {
    return;
  }

  if (!is_down) {
    if (test_mode_ == kTestStart) {
      start_pos_ = terrain_->get_cursor_location();
      start_set_ = true;
    } else if (test_mode_ == kTestEnd) {
      end_pos_ = terrain_->get_cursor_location();
      end_set_ = true;
    }

    find_path();
  }
}

void PathingTool::find_path() {
  if (!start_set_ || !end_set_)
    return;

  std::vector<fw::Vector> full_path;
  if (!path_find_->find(full_path, start_pos_, end_pos_)) {
    statusbar->set_message((boost::format("No path found after %1%ms") % (path_find_->total_time * 1000.0f)).str());
  } else {
    std::vector<fw::Vector> path;
    path_find_->simplify_path(full_path, path);
    statusbar->set_message((boost::format("Path found in %1%ms, %2% nodes, %3% nodes (simplified)")
        % (path_find_->total_time * 1000.0f)
        % full_path.size()
        % path.size()).str());

    std::vector<fw::vertex::xyz_c> buffer;

    if (simplify_) {
      for(fw::Vector loc : path) {
        float height = get_terrain()->get_vertex_height(static_cast<int>(loc[0]), static_cast<int>(loc[2]));
        buffer.push_back(fw::vertex::xyz_c(loc[0], height + 0.2f, loc[2], fw::Color(1, 0.5f, 0.5f, 1)));
      }
    } else {
      for(fw::Vector loc : full_path) {
        float height = get_terrain()->get_vertex_height(static_cast<int>(loc[0]), static_cast<int>(loc[2]));
        buffer.push_back(fw::vertex::xyz_c(loc[0], height + 0.2f, loc[2], fw::Color(1, 0.5f, 0.5f, 1)));
      }
    }

    std::shared_ptr<fw::VertexBuffer> vb = fw::VertexBuffer::create<fw::vertex::xyz_c>();
    vb->set_data(buffer.size(), &buffer[0]);

    current_path_vb = vb;

    current_path_ib = std::shared_ptr<fw::IndexBuffer>(new fw::IndexBuffer());
    std::vector<uint16_t> indices(buffer.size());
    for (int i = 0; i < buffer.size(); i++) {
      indices[i] = i;
    }
    current_path_ib->set_data(indices.size(), indices.data());
  }
}

}
